#ifndef VTL_PUBLICATION_TEXT_OP_DISCORD_DATA_H
#define VTL_PUBLICATION_TEXT_OP_DISCORD_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <VTL/VTL_app_result.h>
#include <stddef.h>

/*
 * Маркеры Discord Markdown.
 *
 * Discord поддерживает:
 *   **text**      — жирный
 *   *text*        — курсив
 *   _text_        — курсив (альтернатива)
 *   ***text***    — жирный + курсив
 *   ~~text~~      — зачёркнутый
 *
 * Внутренний формат проекта поддерживает BOLD, ITALIC, STRIKETHROUGH
 * через VTL_TEXT_MODIFICATION_* флаги — один в один совпадает.
 */
typedef enum _VTL_discord_MarkerKind {
    VTL_discord_marker_kBoldStart = 0,
    VTL_discord_marker_kBoldEnd,
    VTL_discord_marker_kItalicStart,
    VTL_discord_marker_kItalicEnd,
    VTL_discord_marker_kStrikeStart,
    VTL_discord_marker_kStrikeEnd,
    VTL_discord_marker_kBoldItalicStart,
    VTL_discord_marker_kBoldItalicEnd
} VTL_discord_MarkerKind;

/* Один найденный токен */
typedef struct _VTL_discord_Marker {
    VTL_discord_MarkerKind kind;
    size_t pos;
    size_t length;  /* длина самого токена (** = 2, *** = 3, ~~ = 2) */
} VTL_discord_Marker;

/* Динамический массив маркеров -- у каждого потока свой */
typedef struct _VTL_discord_MarkerList {
    VTL_discord_Marker *items;
    size_t length;
    size_t capacity;
} VTL_discord_MarkerList;

typedef struct _VTL_discord_ScanContext {
    const char *text;
    size_t text_length;
    VTL_discord_MarkerList *out;
} VTL_discord_ScanContext;

VTL_AppResult VTL_discord_MarkerListInit(VTL_discord_MarkerList *list);
VTL_AppResult VTL_discord_MarkerListReserve(VTL_discord_MarkerList *list, size_t capacity);
VTL_AppResult VTL_discord_MarkerListPush(VTL_discord_MarkerList *list, VTL_discord_Marker marker);
void VTL_discord_MarkerListFree(VTL_discord_MarkerList *list);

#ifdef __cplusplus
}
#endif

#endif /* VTL_PUBLICATION_TEXT_OP_DISCORD_DATA_H */
