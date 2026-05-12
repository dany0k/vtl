/*
 * Реализация парсера AsciiDoc.
 *
 * Структура файла:
 *   1. Утилиты для MarkerList (динамический массив маркеров)
 *   2. Вспомогательные функции (проверка границ слова, поиск конца строки)
 *   3. Inline-сканеры (bold, italic, mono, sub, sup, strike)
 *   4. Block-сканеры (heading, list, code-block, quoted-block, comment)
 *   5. Специальные сканеры (link, cross-reference, attribute, admonition)
 *   6. Сортировка и слияние маркеров
 *   7. Оркестраторы (Sequential / Parallel)
 *   8. Конвертация в VTL_publication_MarkedText
 *   9. Точки входа для текста и файлов
 *   10. Бенчмарк
 *
 * Главный принцип: каждый сканер пишет только в свой буфер маркеров.
 * Это значит, что мы можем запустить их в параллельных потоках без
 * мьютексов и условных переменных.
 */

#define _POSIX_C_SOURCE 200809L

#include <VTL/publication/text/asciidoc/VTL_publication_text_op_asciidoc.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>


/* Начальная ёмкость буфера маркеров — растёт удвоением при необходимости */
#define VTL_ASCIIDOC_INITIAL_CAPACITY 64

/* Сколько типов разметки мы умеем парсить — столько и потоков создаём */
#define VTL_ASCIIDOC_SCANNER_COUNT 15

/* Открывающая последовательность для зачёркивания: [line-through]# */
static const char VTL_ASCIIDOC_STRIKE_OPEN[] = "[line-through]#";
#define VTL_ASCIIDOC_STRIKE_OPEN_LEN (sizeof(VTL_ASCIIDOC_STRIKE_OPEN) - 1)


/* ============================================================
 * 1. Утилиты для MarkerList
 * ============================================================ */

VTL_AppResult VTL_asciidoc_MarkerListInit(VTL_asciidoc_MarkerList* list)
{
    if (!list) {
        return VTL_res_kInvalidParamErr;
    }
    list->items = NULL;
    list->length = 0;
    list->capacity = 0;
    return VTL_res_kOk;
}

VTL_AppResult VTL_asciidoc_MarkerListReserve(VTL_asciidoc_MarkerList* list, size_t capacity)
{
    if (!list) {
        return VTL_res_kInvalidParamErr;
    }
    if (capacity <= list->capacity) {
        return VTL_res_kOk;
    }
    VTL_asciidoc_Marker* new_items = (VTL_asciidoc_Marker*)realloc(
        list->items, capacity * sizeof(VTL_asciidoc_Marker));
    if (!new_items) {
        return VTL_res_kMemAllocErr;
    }
    list->items = new_items;
    list->capacity = capacity;
    return VTL_res_kOk;
}

VTL_AppResult VTL_asciidoc_MarkerListPush(VTL_asciidoc_MarkerList* list,
                                          VTL_asciidoc_Marker marker)
{
    if (!list) {
        return VTL_res_kInvalidParamErr;
    }
    /* Если место кончилось — удваиваем ёмкость. Это даёт амортизированную
     * сложность O(1) на одну вставку даже при росте массива. */
    if (list->length >= list->capacity) {
        size_t new_cap = list->capacity ? list->capacity * 2 : VTL_ASCIIDOC_INITIAL_CAPACITY;
        VTL_AppResult res = VTL_asciidoc_MarkerListReserve(list, new_cap);
        if (res != VTL_res_kOk) {
            return res;
        }
    }
    list->items[list->length++] = marker;
    return VTL_res_kOk;
}

void VTL_asciidoc_MarkerListClear(VTL_asciidoc_MarkerList* list)
{
    if (list) {
        list->length = 0;
    }
}

void VTL_asciidoc_MarkerListFree(VTL_asciidoc_MarkerList* list)
{
    if (!list) {
        return;
    }
    free(list->items);
    list->items = NULL;
    list->length = 0;
    list->capacity = 0;
}


/* ============================================================
 * 2. Вспомогательные функции для сканеров
 * ============================================================ */

/*
 * Считаем буквы, цифры и подчёркивание "буквенными" символами.
 * Нужно для проверки: стоит ли разметка вплотную к слову или отдельно от него.
 * Например, *bold* — отдельно, перед ним пробел (или начало строки),
 * а 1*2*3 — это арифметика, не разметка.
 */
static bool VTL_asciidoc_is_word_char(char c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
           c == '_';
}

/* Истина, если позиция pos — начало строки (либо самое начало текста,
 * либо сразу после переноса строки). */
static bool VTL_asciidoc_is_at_line_start(const char* text, size_t pos)
{
    if (pos == 0) {
        return true;
    }
    return text[pos - 1] == '\n';
}

/* Возвращает позицию символа '\n' или конец текста, начиная поиск с from. */
static size_t VTL_asciidoc_line_end(const char* text, size_t text_len, size_t from)
{
    size_t i = from;
    while (i < text_len && text[i] != '\n') {
        ++i;
    }
    return i;
}


/* ============================================================
 * 3. Inline-сканеры (парные токены)
 *
 * В AsciiDoc *bold*, _italic_, `mono`, ~sub~, ^sup^ работают одинаково:
 * один и тот же символ — и открывающий, и закрывающий.
 * Правила (упрощённо):
 *   - открывающий символ стоит после пробела/начала строки/знака препинания
 *   - закрывающий стоит перед пробелом/концом строки/знаком препинания
 *   - между ними не может быть пустоты после открывающего и перед закрывающим
 * ============================================================ */

/*
 * Универсальная функция для парных токенов одного символа.
 * Принимает символ-разделитель и пару kinds (открывающий/закрывающий).
 * Возвращает (void*)1 при ошибке аллокации, иначе NULL.
 */
static void* VTL_asciidoc_scan_inline_pair(VTL_asciidoc_ScanContext* ctx,
                                            char delim,
                                            VTL_asciidoc_MarkerKind start_kind,
                                            VTL_asciidoc_MarkerKind end_kind)
{
    /* Флаг "сейчас мы внутри парных токенов" */
    bool open = false;

    for (size_t i = 0; i < ctx->text_length; ++i) {
        if (ctx->text[i] != delim) {
            continue;
        }

        /* Соседние символы. Если их нет — считаем, что там перевод строки. */
        char prev = (i > 0) ? ctx->text[i - 1] : '\n';
        char next = (i + 1 < ctx->text_length) ? ctx->text[i + 1] : '\n';

        if (!open) {
            /* Ищем открывающий: перед ним не должно быть буквы (слова),
             * а после — не должен идти пробел (то есть текст разметки начинается). */
            bool prev_ok = !VTL_asciidoc_is_word_char(prev);
            bool next_ok = next != ' ' && next != '\t' && next != '\n' && next != '\0';
            if (prev_ok && next_ok) {
                VTL_asciidoc_Marker m = { start_kind, i, 1, 0 };
                if (VTL_asciidoc_MarkerListPush(ctx->out, m) != VTL_res_kOk) {
                    return (void*)1;
                }
                open = true;
            }
        } else {
            /* Ищем закрывающий: перед ним не должно быть пробела (текст
             * разметки не "висит в воздухе"), а после — не должна идти буква. */
            bool prev_ok = prev != ' ' && prev != '\t' && prev != '\n';
            bool next_ok = !VTL_asciidoc_is_word_char(next);
            if (prev_ok && next_ok) {
                VTL_asciidoc_Marker m = { end_kind, i, 1, 0 };
                if (VTL_asciidoc_MarkerListPush(ctx->out, m) != VTL_res_kOk) {
                    return (void*)1;
                }
                open = false;
            }
        }
    }
    return NULL;
}


/* Каждый из этих сканеров — это просто специализация общей функции
 * с конкретным символом-разделителем и парой kinds. */

void* VTL_asciidoc_ScanBold(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;
    return VTL_asciidoc_scan_inline_pair(ctx, '*',
        VTL_asciidoc_marker_kBoldStart, VTL_asciidoc_marker_kBoldEnd);
}

void* VTL_asciidoc_ScanItalic(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;
    return VTL_asciidoc_scan_inline_pair(ctx, '_',
        VTL_asciidoc_marker_kItalicStart, VTL_asciidoc_marker_kItalicEnd);
}

void* VTL_asciidoc_ScanMono(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;
    return VTL_asciidoc_scan_inline_pair(ctx, '`',
        VTL_asciidoc_marker_kMonoStart, VTL_asciidoc_marker_kMonoEnd);
}

void* VTL_asciidoc_ScanSubscript(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;
    return VTL_asciidoc_scan_inline_pair(ctx, '~',
        VTL_asciidoc_marker_kSubscriptStart, VTL_asciidoc_marker_kSubscriptEnd);
}

void* VTL_asciidoc_ScanSuperscript(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;
    return VTL_asciidoc_scan_inline_pair(ctx, '^',
        VTL_asciidoc_marker_kSuperscriptStart, VTL_asciidoc_marker_kSuperscriptEnd);
}


/* ============================================================
 * 4. Зачёркивание [line-through]#текст#
 *
 * Не парный одиночный токен, поэтому отдельная функция:
 * ищем последовательность "[line-through]#" и затем первый "#" в этой же строке.
 * ============================================================ */

void* VTL_asciidoc_ScanStrikethrough(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;

    size_t i = 0;
    while (i + VTL_ASCIIDOC_STRIKE_OPEN_LEN <= ctx->text_length) {
        /* Сравниваем 15 байт с эталоном "[line-through]#" */
        if (memcmp(ctx->text + i, VTL_ASCIIDOC_STRIKE_OPEN,
                   VTL_ASCIIDOC_STRIKE_OPEN_LEN) != 0) {
            ++i;
            continue;
        }

        /* Нашли открывающую последовательность — теперь ищем закрывающий # */
        size_t close_pos = (size_t)-1;
        for (size_t j = i + VTL_ASCIIDOC_STRIKE_OPEN_LEN; j < ctx->text_length; ++j) {
            if (ctx->text[j] == '#') { close_pos = j; break; }
            if (ctx->text[j] == '\n') break;   /* за строку не выходим */
        }
        if (close_pos == (size_t)-1) {
            ++i;
            continue;
        }

        /* Маркер start "длинный" (15 байт), end — один символ # */
        VTL_asciidoc_Marker start_m = {
            VTL_asciidoc_marker_kStrikeStart, i, VTL_ASCIIDOC_STRIKE_OPEN_LEN, 0
        };
        VTL_asciidoc_Marker end_m = {
            VTL_asciidoc_marker_kStrikeEnd, close_pos, 1, 0
        };
        if (VTL_asciidoc_MarkerListPush(ctx->out, start_m) != VTL_res_kOk) return (void*)1;
        if (VTL_asciidoc_MarkerListPush(ctx->out, end_m) != VTL_res_kOk) return (void*)1;
        i = close_pos + 1;
    }
    return NULL;
}


/* ============================================================
 * 5. Ссылки и cross-reference
 *
 * link:url[текст]            — обычная ссылка
 * <<id>>  или <<id,текст>>    — внутренняя ссылка на якорь
 *
 * Здесь нас интересует только сам факт ссылки, чтобы при конвертации
 * в MarkedText мы могли её вырезать (если не поддерживаем) или преобразовать
 * (если поддерживаем). Пока вырезаем — внутренний формат не имеет ссылок.
 * ============================================================ */

void* VTL_asciidoc_ScanLink(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;

    static const char prefix[] = "link:";
    const size_t prefix_len = sizeof(prefix) - 1;

    size_t i = 0;
    while (i + prefix_len <= ctx->text_length) {
        /* Ищем "link:" в тексте */
        if (memcmp(ctx->text + i, prefix, prefix_len) != 0) {
            ++i;
            continue;
        }
        /* После префикса идёт url до '[', потом закрывающая ']' */
        size_t bracket_open = (size_t)-1;
        for (size_t j = i + prefix_len; j < ctx->text_length; ++j) {
            if (ctx->text[j] == '[') { bracket_open = j; break; }
            if (ctx->text[j] == ' ' || ctx->text[j] == '\n') break;
        }
        if (bracket_open == (size_t)-1) { ++i; continue; }

        size_t bracket_close = (size_t)-1;
        for (size_t j = bracket_open + 1; j < ctx->text_length; ++j) {
            if (ctx->text[j] == ']') { bracket_close = j; break; }
            if (ctx->text[j] == '\n') break;
        }
        if (bracket_close == (size_t)-1) { ++i; continue; }

        /* Маркер охватывает всё: link:url[текст] */
        VTL_asciidoc_Marker m = {
            VTL_asciidoc_marker_kLink, i, bracket_close - i + 1, 0
        };
        if (VTL_asciidoc_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
        i = bracket_close + 1;
    }
    return NULL;
}

void* VTL_asciidoc_ScanCrossReference(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;

    size_t i = 0;
    while (i + 1 < ctx->text_length) {
        /* Ищем <<  — две угловых скобки подряд */
        if (ctx->text[i] != '<' || ctx->text[i + 1] != '<') {
            ++i;
            continue;
        }
        /* Ищем закрывающее >> */
        size_t close_pos = (size_t)-1;
        for (size_t j = i + 2; j + 1 < ctx->text_length; ++j) {
            if (ctx->text[j] == '>' && ctx->text[j + 1] == '>') {
                close_pos = j + 1;
                break;
            }
            if (ctx->text[j] == '\n') break;
        }
        if (close_pos == (size_t)-1) { ++i; continue; }

        VTL_asciidoc_Marker m = {
            VTL_asciidoc_marker_kCrossReference, i, close_pos - i + 1, 0
        };
        if (VTL_asciidoc_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
        i = close_pos + 1;
    }
    return NULL;
}


/* ============================================================
 * 6. Блочные сканеры (работают по строкам)
 *
 * Все они идут по тексту построчно. Если строка не интересует — пропускают её
 * и переходят к следующей через VTL_asciidoc_line_end + 1.
 * ============================================================ */

void* VTL_asciidoc_ScanHeading(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;

    size_t i = 0;
    while (i < ctx->text_length) {
        /* Заголовок может начинаться только в начале строки */
        if (!VTL_asciidoc_is_at_line_start(ctx->text, i)) {
            i = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i) + 1;
            continue;
        }
        /* Считаем подряд идущие '=' — это уровень заголовка (1..6) */
        size_t level = 0;
        while (i + level < ctx->text_length && ctx->text[i + level] == '=' && level < 6) {
            ++level;
        }
        /* После '=' должен идти пробел, иначе это не заголовок */
        if (level > 0 && i + level < ctx->text_length && ctx->text[i + level] == ' ') {
            VTL_asciidoc_Marker m = {
                VTL_asciidoc_marker_kHeading, i, level + 1, (int)level
            };
            if (VTL_asciidoc_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
        }
        i = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i) + 1;
    }
    return NULL;
}

void* VTL_asciidoc_ScanList(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;

    size_t i = 0;
    while (i < ctx->text_length) {
        if (!VTL_asciidoc_is_at_line_start(ctx->text, i)) {
            i = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i) + 1;
            continue;
        }
        /* Список может начинаться с '*' (маркированный) или '.' (нумерованный).
         * Количество повторов задаёт уровень вложенности. */
        char bullet = ctx->text[i];
        if (bullet == '*' || bullet == '.') {
            size_t level = 0;
            while (i + level < ctx->text_length && ctx->text[i + level] == bullet && level < 5) {
                ++level;
            }
            /* После символов должен идти пробел */
            if (level > 0 && i + level < ctx->text_length && ctx->text[i + level] == ' ') {
                VTL_asciidoc_Marker m = {
                    VTL_asciidoc_marker_kListItem, i, level + 1, (int)level
                };
                if (VTL_asciidoc_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
            }
        }
        i = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i) + 1;
    }
    return NULL;
}

/*
 * Code block обрамляется строкой ровно из четырёх дефисов ----.
 * Открывающая и закрывающая строки одинаковые — поэтому смотрим на состояние
 * "уже внутри блока?" и переключаем его. */
void* VTL_asciidoc_ScanCodeBlock(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;

    size_t i = 0;
    bool open = false;
    while (i < ctx->text_length) {
        if (!VTL_asciidoc_is_at_line_start(ctx->text, i)) {
            i = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i) + 1;
            continue;
        }
        size_t line_end = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i);
        size_t line_len = line_end - i;
        if (line_len == 4 && memcmp(ctx->text + i, "----", 4) == 0) {
            VTL_asciidoc_Marker m = {
                open ? VTL_asciidoc_marker_kCodeBlockEnd
                     : VTL_asciidoc_marker_kCodeBlockStart,
                i, 4, 0
            };
            if (VTL_asciidoc_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
            open = !open;
        }
        i = line_end + 1;
    }
    return NULL;
}

/* Цитатный блок ____ ... ____ — структура такая же, как у code block */
void* VTL_asciidoc_ScanQuotedBlock(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;

    size_t i = 0;
    bool open = false;
    while (i < ctx->text_length) {
        if (!VTL_asciidoc_is_at_line_start(ctx->text, i)) {
            i = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i) + 1;
            continue;
        }
        size_t line_end = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i);
        size_t line_len = line_end - i;
        if (line_len == 4 && memcmp(ctx->text + i, "____", 4) == 0) {
            VTL_asciidoc_Marker m = {
                open ? VTL_asciidoc_marker_kQuotedBlockEnd
                     : VTL_asciidoc_marker_kQuotedBlockStart,
                i, 4, 0
            };
            if (VTL_asciidoc_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
            open = !open;
        }
        i = line_end + 1;
    }
    return NULL;
}

/* Комментарий: строка, начинающаяся с // — её содержимое до конца строки
 * вырезается из вывода. */
void* VTL_asciidoc_ScanComment(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;

    size_t i = 0;
    while (i < ctx->text_length) {
        if (!VTL_asciidoc_is_at_line_start(ctx->text, i)) {
            i = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i) + 1;
            continue;
        }
        if (i + 1 < ctx->text_length && ctx->text[i] == '/' && ctx->text[i + 1] == '/') {
            size_t line_end = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i);
            VTL_asciidoc_Marker m = {
                VTL_asciidoc_marker_kComment, i, line_end - i, 0
            };
            if (VTL_asciidoc_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
            i = line_end + 1;
            continue;
        }
        i = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i) + 1;
    }
    return NULL;
}


/* ============================================================
 * 7. Сканеры уровня документа
 *
 * Атрибуты — это строки вида ":название: значение", обычно в шапке.
 * Admonition paragraphs — строки вида "NOTE: текст", "WARNING: текст" и т.п.
 * ============================================================ */

void* VTL_asciidoc_ScanAttribute(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;

    size_t i = 0;
    while (i < ctx->text_length) {
        if (!VTL_asciidoc_is_at_line_start(ctx->text, i)) {
            i = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i) + 1;
            continue;
        }
        /* Строка должна начинаться с ':' и содержать второе ':' до конца строки */
        if (ctx->text[i] != ':') {
            i = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i) + 1;
            continue;
        }
        size_t line_end = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i);
        size_t second_colon = (size_t)-1;
        for (size_t j = i + 1; j < line_end; ++j) {
            if (ctx->text[j] == ':') { second_colon = j; break; }
        }
        /* Имя атрибута не должно быть пустым, и второе двоеточие должно быть */
        if (second_colon != (size_t)-1 && second_colon > i + 1) {
            VTL_asciidoc_Marker m = {
                VTL_asciidoc_marker_kAttribute, i, line_end - i, 0
            };
            if (VTL_asciidoc_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
        }
        i = line_end + 1;
    }
    return NULL;
}

/* Поддерживаем стандартные admonition: NOTE, TIP, IMPORTANT, WARNING, CAUTION */
static bool VTL_asciidoc_is_admonition_label(const char* text, size_t pos, size_t text_len)
{
    static const char* labels[] = {
        "NOTE: ", "TIP: ", "IMPORTANT: ", "WARNING: ", "CAUTION: "
    };
    static const size_t lens[] = { 6, 5, 11, 9, 9 };
    const size_t count = sizeof(labels) / sizeof(labels[0]);

    for (size_t k = 0; k < count; ++k) {
        if (pos + lens[k] <= text_len &&
            memcmp(text + pos, labels[k], lens[k]) == 0) {
            return true;
        }
    }
    return false;
}

void* VTL_asciidoc_ScanAdmonition(void* arg)
{
    VTL_asciidoc_ScanContext* ctx = (VTL_asciidoc_ScanContext*)arg;
    if (!ctx || !ctx->text || !ctx->out) return (void*)1;

    size_t i = 0;
    while (i < ctx->text_length) {
        if (!VTL_asciidoc_is_at_line_start(ctx->text, i)) {
            i = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i) + 1;
            continue;
        }
        if (VTL_asciidoc_is_admonition_label(ctx->text, i, ctx->text_length)) {
            /* Длина маркера — до двоеточия включительно с пробелом.
             * Это нужно, чтобы при конвертации не выводить "NOTE: " как обычный текст. */
            size_t colon = i;
            while (colon < ctx->text_length && ctx->text[colon] != ':') ++colon;
            size_t marker_len = (colon + 2 <= ctx->text_length) ? (colon - i + 2) : (colon - i + 1);
            VTL_asciidoc_Marker m = {
                VTL_asciidoc_marker_kAdmonition, i, marker_len, 0
            };
            if (VTL_asciidoc_MarkerListPush(ctx->out, m) != VTL_res_kOk) return (void*)1;
        }
        i = VTL_asciidoc_line_end(ctx->text, ctx->text_length, i) + 1;
    }
    return NULL;
}


/* ============================================================
 * 8. Сортировка и слияние маркеров
 *
 * После того как все сканеры закончили, у нас N отдельных списков маркеров
 * (по одному на каждый сканер). Их нужно соединить и отсортировать
 * по позиции — чтобы конвертация шла слева направо одним проходом.
 * ============================================================ */

/* Компаратор для qsort: сначала по позиции, при равной позиции — по kind. */
static int VTL_asciidoc_marker_cmp(const void* a, const void* b)
{
    const VTL_asciidoc_Marker* ma = (const VTL_asciidoc_Marker*)a;
    const VTL_asciidoc_Marker* mb = (const VTL_asciidoc_Marker*)b;
    if (ma->pos < mb->pos) return -1;
    if (ma->pos > mb->pos) return  1;
    if (ma->kind < mb->kind) return -1;
    if (ma->kind > mb->kind) return  1;
    return 0;
}

VTL_AppResult VTL_asciidoc_SortMarkers(VTL_asciidoc_MarkerList* list)
{
    if (!list) return VTL_res_kInvalidParamErr;
    if (list->length > 1) {
        qsort(list->items, list->length, sizeof(VTL_asciidoc_Marker),
              VTL_asciidoc_marker_cmp);
    }
    return VTL_res_kOk;
}

VTL_AppResult VTL_asciidoc_MergeMarkers(const VTL_asciidoc_MarkerList* parts,
                                        size_t parts_count,
                                        VTL_asciidoc_MarkerList* out)
{
    if (!parts || !out) return VTL_res_kInvalidParamErr;

    /* Сначала считаем общий размер, чтобы выделить память сразу — без realloc'ов */
    size_t total = 0;
    for (size_t i = 0; i < parts_count; ++i) {
        total += parts[i].length;
    }
    VTL_AppResult res = VTL_asciidoc_MarkerListReserve(out, total);
    if (res != VTL_res_kOk) return res;

    /* Копируем содержимое каждого частичного списка в общий */
    for (size_t i = 0; i < parts_count; ++i) {
        for (size_t j = 0; j < parts[i].length; ++j) {
            res = VTL_asciidoc_MarkerListPush(out, parts[i].items[j]);
            if (res != VTL_res_kOk) return res;
        }
    }
    return VTL_asciidoc_SortMarkers(out);
}


/* ============================================================
 * 9. Оркестраторы (Sequential / Parallel)
 *
 * Sequential: вызываем все 15 сканеров по очереди, в один поток.
 * Parallel: создаём 15 потоков через pthread, ждём их завершения, сливаем.
 * Один и тот же массив указателей на функции используется в обоих случаях.
 * ============================================================ */

typedef void* (*VTL_asciidoc_scanner_fn)(void*);

/* Список всех сканеров проекта. Если добавляешь новый — допиши сюда. */
static const VTL_asciidoc_scanner_fn VTL_asciidoc_all_scanners[VTL_ASCIIDOC_SCANNER_COUNT] = {
    VTL_asciidoc_ScanBold,
    VTL_asciidoc_ScanItalic,
    VTL_asciidoc_ScanMono,
    VTL_asciidoc_ScanSubscript,
    VTL_asciidoc_ScanSuperscript,
    VTL_asciidoc_ScanStrikethrough,
    VTL_asciidoc_ScanLink,
    VTL_asciidoc_ScanCrossReference,
    VTL_asciidoc_ScanHeading,
    VTL_asciidoc_ScanList,
    VTL_asciidoc_ScanCodeBlock,
    VTL_asciidoc_ScanQuotedBlock,
    VTL_asciidoc_ScanComment,
    VTL_asciidoc_ScanAttribute,
    VTL_asciidoc_ScanAdmonition
};


VTL_AppResult VTL_asciidoc_ParseDocumentSequential(const VTL_publication_Text* src,
                                                    VTL_asciidoc_MarkerList* out)
{
    if (!src || !src->text || !out) return VTL_res_kInvalidParamErr;

    VTL_asciidoc_ScanContext ctx = { src->text, src->length, out };
    for (size_t i = 0; i < VTL_ASCIIDOC_SCANNER_COUNT; ++i) {
        if (VTL_asciidoc_all_scanners[i](&ctx) != NULL) {
            return VTL_res_kErr;
        }
    }
    return VTL_asciidoc_SortMarkers(out);
}


VTL_AppResult VTL_asciidoc_ParseDocumentParallel(const VTL_publication_Text* src,
                                                  VTL_asciidoc_MarkerList* out)
{
    if (!src || !src->text || !out) return VTL_res_kInvalidParamErr;

    /* У каждого сканера — свой буфер маркеров и свой контекст.
     * Это и есть ключ к работе без блокировок: никаких общих записей. */
    VTL_asciidoc_MarkerList partial_lists[VTL_ASCIIDOC_SCANNER_COUNT];
    VTL_asciidoc_ScanContext contexts[VTL_ASCIIDOC_SCANNER_COUNT];
    pthread_t threads[VTL_ASCIIDOC_SCANNER_COUNT];

    for (size_t i = 0; i < VTL_ASCIIDOC_SCANNER_COUNT; ++i) {
        VTL_asciidoc_MarkerListInit(&partial_lists[i]);
        contexts[i].text = src->text;
        contexts[i].text_length = src->length;
        contexts[i].out = &partial_lists[i];
    }

    /* Запускаем потоки. Если pthread_create по какой-то причине не сработает
     * для части потоков — оставшиеся сканеры выполним последовательно. */
    size_t spawned = 0;
    for (size_t i = 0; i < VTL_ASCIIDOC_SCANNER_COUNT; ++i) {
        if (pthread_create(&threads[i], NULL,
                           VTL_asciidoc_all_scanners[i], &contexts[i]) != 0) {
            break;
        }
        ++spawned;
    }
    for (size_t i = spawned; i < VTL_ASCIIDOC_SCANNER_COUNT; ++i) {
        VTL_asciidoc_all_scanners[i](&contexts[i]);
    }

    /* Ждём завершения каждого запущенного потока и собираем коды возврата */
    void* worst = NULL;
    for (size_t i = 0; i < spawned; ++i) {
        void* res = NULL;
        pthread_join(threads[i], &res);
        if (res != NULL) worst = res;
    }

    /* Сливаем частичные буферы в общий и сортируем по позиции */
    VTL_AppResult merge_res = VTL_asciidoc_MergeMarkers(partial_lists,
                                                       VTL_ASCIIDOC_SCANNER_COUNT, out);

    for (size_t i = 0; i < VTL_ASCIIDOC_SCANNER_COUNT; ++i) {
        VTL_asciidoc_MarkerListFree(&partial_lists[i]);
    }

    if (worst != NULL) return VTL_res_kErr;
    return merge_res;
}


/* ============================================================
 * 10. Конвертация в VTL_publication_MarkedText
 *
 * Идём по отсортированным маркерам слева направо. Поддерживаем "курсор" —
 * это позиция в исходном тексте, до которой мы уже записали содержимое.
 * Перед каждым маркером выкидываем накопленный текст с текущими флагами,
 * затем обновляем флаги по типу маркера и перескакиваем через сам маркер.
 * ============================================================ */

static VTL_publication_marked_text_Part VTL_asciidoc_make_part(const char* text_start,
    size_t length, VTL_publication_text_modification_Flags flags)
{
    VTL_publication_marked_text_Part p;
    p.text = (VTL_publication_text_Symbol*)text_start;
    p.length = length;
    p.type = flags;
    return p;
}

VTL_AppResult VTL_asciidoc_ConvertToMarkedText(const VTL_publication_Text* src,
                                                const VTL_asciidoc_MarkerList* markers,
                                                VTL_publication_MarkedText** out)
{
    if (!src || !src->text || !markers || !out) return VTL_res_kInvalidParamErr;

    /* Выделяем блок и сразу самый большой возможный массив частей.
     * Между каждой парой маркеров может быть кусок текста, плюс хвост и пролог. */
    VTL_publication_MarkedText* block =
        (VTL_publication_MarkedText*)malloc(sizeof(VTL_publication_MarkedText));
    if (!block) return VTL_res_kMemAllocErr;

    size_t max_parts = markers->length * 2 + 2;
    block->parts = (VTL_publication_marked_text_Part*)malloc(
        max_parts * sizeof(VTL_publication_marked_text_Part));
    if (!block->parts) { free(block); return VTL_res_kMemAllocErr; }
    block->length = 0;

    /* flags — текущее состояние inline-разметки (BOLD/ITALIC/STRIKETHROUGH).
     * cursor — куда мы дошли в исходном тексте. */
    VTL_publication_text_modification_Flags flags = 0;
    size_t cursor = 0;

    for (size_t i = 0; i < markers->length; ++i) {
        const VTL_asciidoc_Marker* m = &markers->items[i];

        /* Если маркер находится внутри уже обработанного куска — пропускаем
         * (так бывает с вложенными конструкциями: heading + комментарий и т.п.). */
        if (m->pos < cursor) continue;

        /* Текст между предыдущей позицией и текущим маркером — обычный кусок */
        if (m->pos > cursor) {
            block->parts[block->length++] = VTL_asciidoc_make_part(
                src->text + cursor, m->pos - cursor, flags);
        }

        /* Обновляем флаги в зависимости от типа маркера.
         * Inline-маркеры меняют флаги; все остальные просто скипают свой токен. */
        switch (m->kind) {
            case VTL_asciidoc_marker_kBoldStart:    flags |=  VTL_TEXT_MODIFICATION_BOLD;          break;
            case VTL_asciidoc_marker_kBoldEnd:      flags &= ~VTL_TEXT_MODIFICATION_BOLD;          break;
            case VTL_asciidoc_marker_kItalicStart:  flags |=  VTL_TEXT_MODIFICATION_ITALIC;        break;
            case VTL_asciidoc_marker_kItalicEnd:    flags &= ~VTL_TEXT_MODIFICATION_ITALIC;        break;
            case VTL_asciidoc_marker_kStrikeStart:  flags |=  VTL_TEXT_MODIFICATION_STRIKETHROUGH; break;
            case VTL_asciidoc_marker_kStrikeEnd:    flags &= ~VTL_TEXT_MODIFICATION_STRIKETHROUGH; break;

            /* У остальных видов разметки нет соответствующего флага в нашем
             * внутреннем формате — поэтому мы их просто "проедаем" токен.
             * Заголовок "= " станет обычным текстом, комментарий пропадёт совсем. */
            default: break;
        }

        /* Перескакиваем через сам токен маркера */
        cursor = m->pos + m->length;
    }

    /* Хвост текста после последнего маркера */
    if (cursor < src->length) {
        block->parts[block->length++] = VTL_asciidoc_make_part(
            src->text + cursor, src->length - cursor, flags);
    }

    *out = block;
    return VTL_res_kOk;
}


/* ============================================================
 * 11. Высокоуровневые точки входа
 *
 * Эти функции — то, что обычно зовут из бизнес-кода. Внутри они просто
 * объединяют этапы: парсинг → конвертация.
 * ============================================================ */

VTL_AppResult VTL_asciidoc_ParseTextSequential(const VTL_publication_Text* src,
                                                VTL_publication_MarkedText** out)
{
    if (!src || !out) return VTL_res_kInvalidParamErr;

    VTL_asciidoc_MarkerList markers;
    VTL_asciidoc_MarkerListInit(&markers);

    VTL_AppResult res = VTL_asciidoc_ParseDocumentSequential(src, &markers);
    if (res == VTL_res_kOk) {
        res = VTL_asciidoc_ConvertToMarkedText(src, &markers, out);
    }
    VTL_asciidoc_MarkerListFree(&markers);
    return res;
}

VTL_AppResult VTL_asciidoc_ParseTextParallel(const VTL_publication_Text* src,
                                              VTL_publication_MarkedText** out)
{
    if (!src || !out) return VTL_res_kInvalidParamErr;

    VTL_asciidoc_MarkerList markers;
    VTL_asciidoc_MarkerListInit(&markers);

    VTL_AppResult res = VTL_asciidoc_ParseDocumentParallel(src, &markers);
    if (res == VTL_res_kOk) {
        res = VTL_asciidoc_ConvertToMarkedText(src, &markers, out);
    }
    VTL_asciidoc_MarkerListFree(&markers);
    return res;
}


/* Читает файл в память и парсит через параллельную версию. */
static VTL_AppResult VTL_asciidoc_load_file(const char* path, char** out_buf, size_t* out_len)
{
    FILE* f = fopen(path, "rb");
    if (!f) return VTL_res_kFileOpenErr;

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return VTL_res_kFileReadErr; }
    long size = ftell(f);
    if (size < 0)                  { fclose(f); return VTL_res_kFileReadErr; }
    rewind(f);

    char* buf = (char*)malloc((size_t)size + 1);
    if (!buf) { fclose(f); return VTL_res_kMemAllocErr; }

    size_t read = fread(buf, 1, (size_t)size, f);
    fclose(f);
    if (read != (size_t)size) { free(buf); return VTL_res_kFileReadErr; }
    buf[size] = '\0';

    *out_buf = buf;
    *out_len = (size_t)size;
    return VTL_res_kOk;
}

VTL_AppResult VTL_asciidoc_ParseFile(const char* path,
                                      VTL_publication_MarkedText** out)
{
    if (!path || !out) return VTL_res_kInvalidParamErr;

    char* buf = NULL;
    size_t len = 0;
    VTL_AppResult res = VTL_asciidoc_load_file(path, &buf, &len);
    if (res != VTL_res_kOk) return res;

    VTL_publication_Text src = { buf, len };
    res = VTL_asciidoc_ParseTextParallel(&src, out);
    free(buf);
    return res;
}


/* ============================================================
 * 12. Batch — несколько файлов сразу
 *
 * Второй уровень параллелизма: когда у нас N файлов, мы запускаем N потоков,
 * и каждый поток обрабатывает свой файл. Внутри файла можно использовать
 * либо параллельную, либо последовательную версию — здесь мы используем
 * параллельную (через ParseFile → ParseTextParallel).
 * ============================================================ */

VTL_AppResult VTL_asciidoc_ParseFileBatchSequential(const char* const* paths,
                                                     size_t paths_count,
                                                     VTL_publication_MarkedText** out_marked)
{
    if (!paths || !out_marked) return VTL_res_kInvalidParamErr;

    for (size_t i = 0; i < paths_count; ++i) {
        out_marked[i] = NULL;
        VTL_AppResult res = VTL_asciidoc_ParseFile(paths[i], &out_marked[i]);
        if (res != VTL_res_kOk) return res;
    }
    return VTL_res_kOk;
}

/* Контекст для одного потока batch — он знает свой файл и куда положить ответ. */
typedef struct _VTL_asciidoc_BatchWorkerCtx
{
    const char* path;
    VTL_publication_MarkedText** slot;
    VTL_AppResult result;
} VTL_asciidoc_BatchWorkerCtx;

static void* VTL_asciidoc_batch_worker(void* arg)
{
    /* Внутри batch-потока используем последовательный парсер.
     * Иначе получится 2 уровня параллелизма (поток-на-файл + 15 сканеров),
     * и реальных рабочих потоков станет в 15 раз больше числа ядер CPU. */
    VTL_asciidoc_BatchWorkerCtx* ctx = (VTL_asciidoc_BatchWorkerCtx*)arg;

    char* buf = NULL;
    size_t len = 0;
    ctx->result = VTL_asciidoc_load_file(ctx->path, &buf, &len);
    if (ctx->result != VTL_res_kOk) return NULL;

    VTL_publication_Text src = { buf, len };
    ctx->result = VTL_asciidoc_ParseTextSequential(&src, ctx->slot);
    free(buf);
    return NULL;
}

VTL_AppResult VTL_asciidoc_ParseFileBatchParallel(const char* const* paths,
                                                   size_t paths_count,
                                                   VTL_publication_MarkedText** out_marked)
{
    if (!paths || !out_marked) return VTL_res_kInvalidParamErr;
    if (paths_count == 0) return VTL_res_kOk;

    pthread_t* threads = (pthread_t*)malloc(paths_count * sizeof(pthread_t));
    VTL_asciidoc_BatchWorkerCtx* ctxs = (VTL_asciidoc_BatchWorkerCtx*)malloc(
        paths_count * sizeof(VTL_asciidoc_BatchWorkerCtx));
    if (!threads || !ctxs) {
        free(threads); free(ctxs);
        return VTL_res_kMemAllocErr;
    }

    for (size_t i = 0; i < paths_count; ++i) {
        out_marked[i] = NULL;
        ctxs[i].path = paths[i];
        ctxs[i].slot = &out_marked[i];
        ctxs[i].result = VTL_res_kOk;
    }

    /* Запускаем по потоку на файл */
    size_t spawned = 0;
    for (size_t i = 0; i < paths_count; ++i) {
        if (pthread_create(&threads[i], NULL,
                           VTL_asciidoc_batch_worker, &ctxs[i]) != 0) {
            break;
        }
        ++spawned;
    }
    for (size_t i = spawned; i < paths_count; ++i) {
        VTL_asciidoc_batch_worker(&ctxs[i]);
    }
    for (size_t i = 0; i < spawned; ++i) {
        pthread_join(threads[i], NULL);
    }

    /* Возвращаем худший статус из всех воркеров (если хоть один упал) */
    VTL_AppResult final_res = VTL_res_kOk;
    for (size_t i = 0; i < paths_count; ++i) {
        if (ctxs[i].result != VTL_res_kOk) {
            final_res = ctxs[i].result;
        }
    }

    free(threads);
    free(ctxs);
    return final_res;
}


/* ============================================================
 * 13. Бенчмарк
 *
 * Замеряем wall-clock через CLOCK_MONOTONIC. Это единственный правильный
 * таймер для параллельного кода: clock() измеряет CPU-time и на N потоках
 * показал бы N-кратное "увеличение", а нам нужно реальное прошедшее время.
 * ============================================================ */

static double VTL_asciidoc_now_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

double VTL_asciidoc_BenchSequential(const VTL_publication_Text* src, size_t iterations)
{
    if (!src || iterations == 0) return 0.0;

    double start = VTL_asciidoc_now_seconds();
    for (size_t i = 0; i < iterations; ++i) {
        VTL_asciidoc_MarkerList markers;
        VTL_asciidoc_MarkerListInit(&markers);
        VTL_asciidoc_ParseDocumentSequential(src, &markers);
        VTL_asciidoc_MarkerListFree(&markers);
    }
    return VTL_asciidoc_now_seconds() - start;
}

double VTL_asciidoc_BenchParallel(const VTL_publication_Text* src, size_t iterations)
{
    if (!src || iterations == 0) return 0.0;

    double start = VTL_asciidoc_now_seconds();
    for (size_t i = 0; i < iterations; ++i) {
        VTL_asciidoc_MarkerList markers;
        VTL_asciidoc_MarkerListInit(&markers);
        VTL_asciidoc_ParseDocumentParallel(src, &markers);
        VTL_asciidoc_MarkerListFree(&markers);
    }
    return VTL_asciidoc_now_seconds() - start;
}
