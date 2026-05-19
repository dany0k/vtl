#ifndef VTL_PUBLICATION_TEXT_OP_DISCORD_SCAN_H
#define VTL_PUBLICATION_TEXT_OP_DISCORD_SCAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <VTL/publication/text/discord/VTL_publication_text_op_discord_data.h>
#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/VTL_app_result.h>

/*
 * Сканеры Discord Markdown.
 *
 * Каждый сканер проходит по всему тексту и ищет свой тип разметки.
 * Возвращает NULL при успехе, (void*)1 при ошибке выделения памяти.
 */

/* ***text*** — жирный курсив (обрабатываем первым — длиннее чем ** и *) */
void *VTL_discord_ScanBoldItalic(void *ctx);
/* **text** — жирный */
void *VTL_discord_ScanBold(void *ctx);
/* *text* — курсив через звёздочку */
void *VTL_discord_ScanItalicStar(void *ctx);
/* _text_ — курсив через подчёркивание */
void *VTL_discord_ScanItalicUnderscore(void *ctx);
/* ~~text~~ — зачёркнутый */
void *VTL_discord_ScanStrikethrough(void *ctx);

#define VTL_DISCORD_SCANNER_COUNT 5

/*
 * Запускает все сканеры и сливает результат в out (отсортированный по позиции).
 * Sequential — один поток, Parallel — каждый сканер в своём потоке.
 */
VTL_AppResult VTL_discord_ParseDocumentSequential(const VTL_publication_Text *src, VTL_discord_MarkerList *out);
VTL_AppResult VTL_discord_ParseDocumentParallel(const VTL_publication_Text *src, VTL_discord_MarkerList *out);

#ifdef __cplusplus
}
#endif

#endif /* VTL_PUBLICATION_TEXT_OP_DISCORD_SCAN_H */
