#ifndef VTL_PUBLICATION_TEXT_OP_DISCORD_H
#define VTL_PUBLICATION_TEXT_OP_DISCORD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <VTL/publication/text/discord/VTL_publication_text_op_discord_data.h>
#include <VTL/publication/text/discord/VTL_publication_text_op_discord_scan.h>
#include <VTL/publication/text/discord/VTL_publication_text_op_discord_convert.h>
#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/VTL_app_result.h>

/*
 * Discord Markdown <-> внутренний формат проекта.
 *
 * Поддерживаемая разметка:
 *   **text**    BOLD
 *   *text*      ITALIC
 *   _text_      ITALIC
 *   ***text***  BOLD | ITALIC
 *   ~~text~~    STRIKETHROUGH
 *
 * Два режима парсинга:
 *   Sequential -- все 5 сканеров в одном потоке
 *   Parallel   -- (pThread) каждый сканер в своём потоке
 */

/* Discord MD -> VTL_publication_MarkedText */
VTL_AppResult VTL_discord_ParseTextSequential(const VTL_publication_Text* src, VTL_publication_MarkedText** out);
VTL_AppResult VTL_discord_ParseTextParallel(const VTL_publication_Text* src, VTL_publication_MarkedText** out);
/* VTL_publication_MarkedText -> Discord MD строка */
VTL_AppResult VTL_discord_SerializeToText(const VTL_publication_MarkedText* src, VTL_publication_Text** out);

#ifdef __cplusplus
}
#endif

#endif /* VTL_PUBLICATION_TEXT_OP_DISCORD_H */
