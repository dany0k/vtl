#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif
#endif

#include <VTL/publication/text/discord/VTL_publication_text_op_discord_scan.h>
#include <VTL/publication/text/discord/VTL_publication_text_op_discord_compat.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* =========================================================================
 * Хелперы
 * ========================================================================= */

static bool vtl_discord_is_word_char(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9');
}

/*
 * Ищет парный токен длиной token_len символов sym.
 * Правила Discord:
 *   открывающий - после него не пробел
 *   закрывающий - перед ним не пробел
 *
 * start_kind / end_kind - что кладём в список при нахождении.
 * Возвращает (void*)1 при OOM, NULL при успехе.
 */
static void *vtl_discord_scan_pair(VTL_discord_ScanContext *ctx, char sym, size_t token_len, VTL_discord_MarkerKind start_kind, VTL_discord_MarkerKind end_kind) {
    const char *text = ctx->text;
    size_t len = ctx->text_length;
    bool open = false;

    size_t i = 0;
    while (i < len) {
        /* Проверяем что с позиции i начинается ровно token_len подряд sym */
        if ((unsigned char) text[i] != (unsigned char) sym) {
            ++i;
            continue;
        }

        /* Считаем сколько sym подряд */
        size_t run = 0;
        while (i + run < len && text[i + run] == sym) ++run;

        /* Нас интересует именно token_len, не больше и не меньше.
         * Для *** мы не хотим совпадать с ** и наоборот.
         * Поэтому пропускаем если run != token_len. */
        if (run != token_len) {
            i += run;
            continue;
        }

        char prev = (i > 0) ? text[i - 1] : '\n';
        char next = (i + run < len) ? text[i + token_len] : '\n';

        if (!open) {
            /* Открывающий: после токена не пробел и не конец строки */
            bool next_ok = next != ' ' && next != '\t' &&
                           next != '\n' && next != '\0';
            if (next_ok) {
                VTL_discord_Marker m = {start_kind, i, token_len};
                if (VTL_discord_MarkerListPush(ctx->out, m) != VTL_res_kOk)
                    return (void *) 1;
                open = true;
            }
        } else {
            /* Закрывающий: перед токеном не пробел */
            bool prev_ok = prev != ' ' && prev != '\t' && prev != '\n';
            if (prev_ok) {
                VTL_discord_Marker m = {end_kind, i, token_len};
                if (VTL_discord_MarkerListPush(ctx->out, m) != VTL_res_kOk)
                    return (void *) 1;
                open = false;
            }
        }
        i += run;
    }
    return NULL;
}

/*
 * Специальный сканер для _курсива_ через подчёркивание.
 * Discord требует чтобы _ не был внутри слова (иначе snake_case ломается).
 * Правило: _ открывает только если перед ним не буква/цифра,
 *           _ закрывает только если после него не буква/цифра.
 */
static void *vtl_discord_scan_underscore_italic(VTL_discord_ScanContext *ctx) {
    const char *text = ctx->text;
    size_t len = ctx->text_length;
    bool open = false;

    for (size_t i = 0; i < len; ++i) {
        if (text[i] != '_') continue;

        /* Пропускаем __ (двойное подчёркивание — underline в Discord,
         * не поддерживается в нашем формате) */
        if (i + 1 < len && text[i + 1] == '_') {
            ++i;
            continue;
        }

        char prev = (i > 0) ? text[i - 1] : '\n';
        char next = (i + 1 < len) ? text[i + 1] : '\n';

        if (!open) {
            bool prev_ok = !vtl_discord_is_word_char(prev);
            bool next_ok = next != ' ' && next != '\t' &&
                           next != '\n' && next != '\0';
            if (prev_ok && next_ok) {
                VTL_discord_Marker m = {
                        VTL_discord_marker_kItalicStart, i, 1
                };
                if (VTL_discord_MarkerListPush(ctx->out, m) != VTL_res_kOk)
                    return (void *) 1;
                open = true;
            }
        } else {
            bool prev_ok = prev != ' ' && prev != '\t' && prev != '\n';
            bool next_ok = !vtl_discord_is_word_char(next);
            if (prev_ok && next_ok) {
                VTL_discord_Marker m = {
                        VTL_discord_marker_kItalicEnd, i, 1
                };
                if (VTL_discord_MarkerListPush(ctx->out, m) != VTL_res_kOk)
                    return (void *) 1;
                open = false;
            }
        }
    }
    return NULL;
}

/* =========================================================================
 * Публичные сканеры — сигнатура void*(void*) для потоков
 * ========================================================================= */
void *VTL_discord_ScanBoldItalic(void *arg) {
    /* ***text*** — три звёздочки. Обрабатываем до ** и * чтобы не пересекаться */
    VTL_discord_ScanContext *ctx = (VTL_discord_ScanContext *) arg;
    return vtl_discord_scan_pair(ctx, '*', 3, VTL_discord_marker_kBoldItalicStart, VTL_discord_marker_kBoldItalicEnd);
}

void *VTL_discord_ScanBold(void *arg) {
    /* **text** — две звёздочки */
    VTL_discord_ScanContext *ctx = (VTL_discord_ScanContext *) arg;
    return vtl_discord_scan_pair(ctx, '*', 2, VTL_discord_marker_kBoldStart, VTL_discord_marker_kBoldEnd);
}

void *VTL_discord_ScanItalicStar(void *arg) {
    /* *text* — одна звёздочка */
    VTL_discord_ScanContext *ctx = (VTL_discord_ScanContext *) arg;
    return vtl_discord_scan_pair(ctx, '*', 1, VTL_discord_marker_kItalicStart, VTL_discord_marker_kItalicEnd);
}

void *VTL_discord_ScanItalicUnderscore(void *arg) {
    VTL_discord_ScanContext *ctx = (VTL_discord_ScanContext *) arg;
    return vtl_discord_scan_underscore_italic(ctx);
}

void *VTL_discord_ScanStrikethrough(void *arg) {
    /* ~~text~~ — две тильды */
    VTL_discord_ScanContext *ctx = (VTL_discord_ScanContext *) arg;
    return vtl_discord_scan_pair(ctx, '~', 2, VTL_discord_marker_kStrikeStart, VTL_discord_marker_kStrikeEnd);
}

/* =========================================================================
 * Таблица всех сканеров -- порядок важен:
 * *** должен идти раньше ** и *, иначе ** поглотит первые две звёздочки ***
 * ========================================================================= */
static void *(*vtl_discord_all_scanners[VTL_DISCORD_SCANNER_COUNT])(void *) = {
        VTL_discord_ScanBoldItalic,
        VTL_discord_ScanBold,
        VTL_discord_ScanItalicStar,
        VTL_discord_ScanItalicUnderscore,
        VTL_discord_ScanStrikethrough,
};

/* =========================================================================
 * Слияние и сортировка маркеров из нескольких буферов
 * ========================================================================= */

static int vtl_discord_marker_cmp(const void *a, const void *b) {
    const VTL_discord_Marker *ma = (const VTL_discord_Marker *) a;
    const VTL_discord_Marker *mb = (const VTL_discord_Marker *) b;
    if (ma->pos < mb->pos) return -1;
    if (ma->pos > mb->pos) return 1;
    /* При равной позиции: Start раньше End */
    if (ma->kind < mb->kind) return -1;
    if (ma->kind > mb->kind) return 1;
    return 0;
}

static VTL_AppResult vtl_discord_merge_and_sort(VTL_discord_MarkerList *parts,
                                                size_t count,
                                                VTL_discord_MarkerList *out) {
    size_t total = 0;
    for (size_t i = 0; i < count; ++i) total += parts[i].length;

    VTL_AppResult res = VTL_discord_MarkerListReserve(out, total);
    if (res != VTL_res_kOk) return res;

    for (size_t i = 0; i < count; ++i) {
        for (size_t j = 0; j < parts[i].length; ++j) {
            res = VTL_discord_MarkerListPush(out, parts[i].items[j]);
            if (res != VTL_res_kOk) return res;
        }
    }

    if (out->length > 1) {
        qsort(out->items, out->length,
              sizeof(VTL_discord_Marker), vtl_discord_marker_cmp);
    }
    return VTL_res_kOk;
}

/* =========================================================================
 * Sequential: все сканеры в одном потоке
 * ========================================================================= */
VTL_AppResult VTL_discord_ParseDocumentSequential(const VTL_publication_Text *src,
                                                  VTL_discord_MarkerList *out) {
    if (!src || !src->text || !out) return VTL_res_kInvalidParamErr;

    VTL_discord_MarkerList partial[VTL_DISCORD_SCANNER_COUNT];
    VTL_discord_ScanContext contexts[VTL_DISCORD_SCANNER_COUNT];

    for (size_t i = 0; i < VTL_DISCORD_SCANNER_COUNT; ++i) {
        VTL_discord_MarkerListInit(&partial[i]);
        contexts[i].text = src->text;
        contexts[i].text_length = src->length;
        contexts[i].out = &partial[i];
    }

    for (size_t i = 0; i < VTL_DISCORD_SCANNER_COUNT; ++i) {
        vtl_discord_all_scanners[i](&contexts[i]);
    }

    VTL_AppResult res = vtl_discord_merge_and_sort(partial, VTL_DISCORD_SCANNER_COUNT, out);

    for (size_t i = 0; i < VTL_DISCORD_SCANNER_COUNT; ++i)
        VTL_discord_MarkerListFree(&partial[i]);

    return res;
}

/* =========================================================================
 * Parallel: каждый сканер в своём потоке (pthread / WinAPI через compat.h)
 *
 * У каждого сканера свой буфер → нет общего состояния → нет локов.
 * Если создание потока падает — оставшиеся гоним последовательно.
 * ========================================================================= */
VTL_AppResult VTL_discord_ParseDocumentParallel(const VTL_publication_Text *src,
                                                VTL_discord_MarkerList *out) {
    if (!src || !src->text || !out) return VTL_res_kInvalidParamErr;

    VTL_discord_MarkerList partial[VTL_DISCORD_SCANNER_COUNT];
    VTL_discord_ScanContext contexts[VTL_DISCORD_SCANNER_COUNT];
    vtl_thread_t threads[VTL_DISCORD_SCANNER_COUNT];

    for (size_t i = 0; i < VTL_DISCORD_SCANNER_COUNT; ++i) {
        VTL_discord_MarkerListInit(&partial[i]);
        contexts[i].text = src->text;
        contexts[i].text_length = src->length;
        contexts[i].out = &partial[i];
    }

    /* Запускаем потоки, при неудаче — fallback на sequential */
    size_t spawned = 0;
    for (size_t i = 0; i < VTL_DISCORD_SCANNER_COUNT; ++i) {
        if (vtl_thread_create(&threads[i],
                              vtl_discord_all_scanners[i],
                              &contexts[i]) != 0) {
            break;
        }
        ++spawned;
    }
    /* Оставшиеся — в текущем потоке */
    for (size_t i = spawned; i < VTL_DISCORD_SCANNER_COUNT; ++i) {
        vtl_discord_all_scanners[i](&contexts[i]);
    }

    /* Ждём все запущенные потоки */
    for (size_t i = 0; i < spawned; ++i) {
        vtl_thread_join(threads[i]);
    }

    VTL_AppResult res = vtl_discord_merge_and_sort(partial, VTL_DISCORD_SCANNER_COUNT, out);

    for (size_t i = 0; i < VTL_DISCORD_SCANNER_COUNT; ++i)
        VTL_discord_MarkerListFree(&partial[i]);

    return res;
}
