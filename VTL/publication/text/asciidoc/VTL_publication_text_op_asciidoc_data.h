#ifndef _VTL_PUBLICATION_TEXT_OP_ASCIIDOC_DATA_H
#define _VTL_PUBLICATION_TEXT_OP_ASCIIDOC_DATA_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/VTL_app_result.h>
#include <stddef.h>


// какой именно тип разметки нашёл сканер
// Start/End — парные токены (открывающий и закрывающий *)
// остальные — одиночные (заголовок, ссылка и т.п.)
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


// один найденный токен
// pos    — где в исходном тексте
// length — сколько байт занимает токен (чтобы выкинуть при конвертации)
// level  — для заголовков 1..6 и вложенности списков
typedef struct _VTL_asciidoc_Marker
{
    VTL_asciidoc_MarkerKind kind;
    size_t pos;
    size_t length;
    int level;
} VTL_asciidoc_Marker;


// динамический массив маркеров, растёт удвоением
// у каждого потока-сканера свой такой буфер — поэтому без локов
typedef struct _VTL_asciidoc_MarkerList
{
    VTL_asciidoc_Marker* items;
    size_t length;
    size_t capacity;
} VTL_asciidoc_MarkerList;


// что сканер получает на вход: куда смотреть и куда складывать
typedef struct _VTL_asciidoc_ScanContext
{
    const char* text;
    size_t text_length;
    VTL_asciidoc_MarkerList* out;
} VTL_asciidoc_ScanContext;


// утилиты для MarkerList
VTL_AppResult VTL_asciidoc_MarkerListInit(VTL_asciidoc_MarkerList* list);
VTL_AppResult VTL_asciidoc_MarkerListReserve(VTL_asciidoc_MarkerList* list, size_t capacity);
VTL_AppResult VTL_asciidoc_MarkerListPush(VTL_asciidoc_MarkerList* list,
                                          VTL_asciidoc_Marker marker);
void VTL_asciidoc_MarkerListClear(VTL_asciidoc_MarkerList* list);
void VTL_asciidoc_MarkerListFree(VTL_asciidoc_MarkerList* list);


#ifdef __cplusplus
}
#endif

#endif
