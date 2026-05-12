#ifndef _VTL_PUBLICATION_TEXT_OP_ASCIIDOC_H
#define _VTL_PUBLICATION_TEXT_OP_ASCIIDOC_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/VTL_app_result.h>
#include <stddef.h>


/*
 * Парсер AsciiDoc → внутренний формат проекта (VTL_publication_MarkedText).
 *
 * Идея работы:
 *   1. Текст пробегают 15 независимых "сканеров" — каждый ищет только свой тип
 *      разметки (один — только *жирный*, другой — только _курсив_ и т.д.).
 *   2. Сканеры не пишут в общую память, у каждого свой буфер маркеров.
 *      Поэтому их можно запускать в параллельных потоках без блокировок.
 *   3. После сканеров буферы сливаются в один список, сортируются по позиции.
 *   4. Один последовательный проход превращает список маркеров в MarkedText:
 *      берёт текст между маркерами и присваивает ему текущие флаги.
 */


/*
 * Виды маркеров. Каждый сканер кладёт в буфер свой тип маркера.
 * Start/End — это "открывающий" и "закрывающий" токен (например, два символа *).
 * Точечные маркеры (Heading, Comment, Link, ...) ставятся один раз на конструкцию.
 */
typedef enum _VTL_asciidoc_MarkerKind
{
    VTL_asciidoc_marker_kBoldStart = 0,
    VTL_asciidoc_marker_kBoldEnd,
    VTL_asciidoc_marker_kItalicStart,
    VTL_asciidoc_marker_kItalicEnd,
    VTL_asciidoc_marker_kMonoStart,
    VTL_asciidoc_marker_kMonoEnd,
    VTL_asciidoc_marker_kSubscriptStart,
    VTL_asciidoc_marker_kSubscriptEnd,
    VTL_asciidoc_marker_kSuperscriptStart,
    VTL_asciidoc_marker_kSuperscriptEnd,
    VTL_asciidoc_marker_kStrikeStart,
    VTL_asciidoc_marker_kStrikeEnd,

    VTL_asciidoc_marker_kHeading,
    VTL_asciidoc_marker_kListItem,
    VTL_asciidoc_marker_kCodeBlockStart,
    VTL_asciidoc_marker_kCodeBlockEnd,
    VTL_asciidoc_marker_kQuotedBlockStart,
    VTL_asciidoc_marker_kQuotedBlockEnd,
    VTL_asciidoc_marker_kComment,

    VTL_asciidoc_marker_kAttribute,
    VTL_asciidoc_marker_kAdmonition,
    VTL_asciidoc_marker_kLink,
    VTL_asciidoc_marker_kCrossReference
} VTL_asciidoc_MarkerKind;


/*
 * Один найденный маркер.
 *   pos    — байтовая позиция в исходном тексте.
 *   length — сколько байт занимает сам токен разметки (нужно при конвертации,
 *            чтобы выкинуть эти байты из выходного текста).
 *   level  — заполняется для маркеров с уровнем (heading 1..6, list 1..3).
 */
typedef struct _VTL_asciidoc_Marker
{
    VTL_asciidoc_MarkerKind kind;
    size_t pos;
    size_t length;
    int level;
} VTL_asciidoc_Marker;


/*
 * Динамический массив маркеров. Растёт по принципу удвоения ёмкости.
 * Каждый поток-сканер пишет в свой собственный MarkerList — это убирает
 * необходимость в мьютексах.
 */
typedef struct _VTL_asciidoc_MarkerList
{
    VTL_asciidoc_Marker* items;
    size_t length;
    size_t capacity;
} VTL_asciidoc_MarkerList;


/*
 * Контекст, который передаётся каждому сканеру как void*. Внутри — указатели
 * на входной текст и на выходной буфер маркеров (свой у каждого сканера).
 */
typedef struct _VTL_asciidoc_ScanContext
{
    const char* text;
    size_t text_length;
    VTL_asciidoc_MarkerList* out;
} VTL_asciidoc_ScanContext;


/* === Утилиты для работы с MarkerList === */

VTL_AppResult VTL_asciidoc_MarkerListInit(VTL_asciidoc_MarkerList* list);
VTL_AppResult VTL_asciidoc_MarkerListReserve(VTL_asciidoc_MarkerList* list, size_t capacity);
VTL_AppResult VTL_asciidoc_MarkerListPush(VTL_asciidoc_MarkerList* list,
                                          VTL_asciidoc_Marker marker);
void VTL_asciidoc_MarkerListClear(VTL_asciidoc_MarkerList* list);
void VTL_asciidoc_MarkerListFree(VTL_asciidoc_MarkerList* list);


/*
 * === Атомарные сканеры ===
 * Сигнатура void*(void*) совместима с pthread — каждый сканер можно либо
 * вызвать напрямую (последовательный режим), либо отдать в pthread_create
 * (параллельный режим). Один и тот же код, два режима использования.
 *
 * Возвращают NULL при успехе и (void*)1 при ошибке (out-of-memory).
 */

/* Inline-разметка (парные токены) */
void* VTL_asciidoc_ScanBold(void* ctx);            /* *текст* */
void* VTL_asciidoc_ScanItalic(void* ctx);          /* _текст_ */
void* VTL_asciidoc_ScanMono(void* ctx);            /* `текст` */
void* VTL_asciidoc_ScanSubscript(void* ctx);       /* ~текст~ */
void* VTL_asciidoc_ScanSuperscript(void* ctx);     /* ^текст^ */
void* VTL_asciidoc_ScanStrikethrough(void* ctx);   /* [line-through]#текст# */

/* Inline-разметка (специальные конструкции) */
void* VTL_asciidoc_ScanLink(void* ctx);            /* link:url[текст] / http(s)://... */
void* VTL_asciidoc_ScanCrossReference(void* ctx);  /* <<id>> или <<id,текст>> */

/* Блочная разметка (line-based) */
void* VTL_asciidoc_ScanHeading(void* ctx);         /* = Заголовок ... ====== */
void* VTL_asciidoc_ScanList(void* ctx);            /* * пункт, . нумерованный */
void* VTL_asciidoc_ScanCodeBlock(void* ctx);       /* ---- ... ---- */
void* VTL_asciidoc_ScanQuotedBlock(void* ctx);     /* ____ ... ____ */
void* VTL_asciidoc_ScanComment(void* ctx);         /* // комментарий до конца строки */

/* Документ-уровень */
void* VTL_asciidoc_ScanAttribute(void* ctx);       /* :ключ: значение */
void* VTL_asciidoc_ScanAdmonition(void* ctx);      /* NOTE:, TIP:, WARNING:, ... */


/* === Оркестрация (запуск всех сканеров) === */

/*
 * Последовательно вызывает каждый сканер. Используется как baseline
 * для бенчмарка и как fallback, если по каким-то причинам потоки недоступны.
 */
VTL_AppResult VTL_asciidoc_ParseDocumentSequential(const VTL_publication_Text* src,
                                                    VTL_asciidoc_MarkerList* out);

/*
 * Запускает все сканеры в параллельных потоках через pthread,
 * ждёт завершения, сливает их буферы в один и сортирует.
 */
VTL_AppResult VTL_asciidoc_ParseDocumentParallel(const VTL_publication_Text* src,
                                                  VTL_asciidoc_MarkerList* out);


/* === Слияние и сортировка маркеров === */

/*
 * Соединяет N частичных списков (от каждого потока) в один общий.
 * Затем сортирует все маркеры по позиции в тексте, чтобы конвертация
 * могла идти одним проходом слева направо.
 */
VTL_AppResult VTL_asciidoc_MergeMarkers(const VTL_asciidoc_MarkerList* parts,
                                        size_t parts_count,
                                        VTL_asciidoc_MarkerList* out);

VTL_AppResult VTL_asciidoc_SortMarkers(VTL_asciidoc_MarkerList* list);


/* === Конвертация в внутренний формат VTL_publication_MarkedText === */

/*
 * Идёт по отсортированным маркерам и формирует массив частей текста.
 * Между маркерами текст записывается как обычный, на границах маркеров
 * меняются флаги (BOLD/ITALIC/STRIKETHROUGH).
 * Маркеры без inline-эффекта (heading, list, comment, link, attribute, ...)
 * приводят к тому, что их байты вырезаются из выходного текста.
 */
VTL_AppResult VTL_asciidoc_ConvertToMarkedText(const VTL_publication_Text* src,
                                                const VTL_asciidoc_MarkerList* markers,
                                                VTL_publication_MarkedText** out);


/* === Высокоуровневые точки входа === */

VTL_AppResult VTL_asciidoc_ParseTextSequential(const VTL_publication_Text* src,
                                                VTL_publication_MarkedText** out);

VTL_AppResult VTL_asciidoc_ParseTextParallel(const VTL_publication_Text* src,
                                              VTL_publication_MarkedText** out);

VTL_AppResult VTL_asciidoc_ParseFile(const char* path,
                                      VTL_publication_MarkedText** out);


/* === Batch (несколько файлов одновременно, поток на файл) === */

VTL_AppResult VTL_asciidoc_ParseFileBatchSequential(const char* const* paths,
                                                     size_t paths_count,
                                                     VTL_publication_MarkedText** out_marked);

VTL_AppResult VTL_asciidoc_ParseFileBatchParallel(const char* const* paths,
                                                   size_t paths_count,
                                                   VTL_publication_MarkedText** out_marked);


/* === Бенчмарк (замер ускорения от распараллеливания) === */

/*
 * Возвращают время в секундах. Замер делается через clock_gettime(MONOTONIC) —
 * это wall-clock, единственный правильный таймер для параллельного кода.
 * iterations — сколько раз повторить парсинг для усреднения.
 */
double VTL_asciidoc_BenchSequential(const VTL_publication_Text* src, size_t iterations);
double VTL_asciidoc_BenchParallel(const VTL_publication_Text* src, size_t iterations);


#ifdef __cplusplus
}
#endif

#endif
