#ifndef VTL_PUBLICATION_TEXT_OP_DISCORD_CONVERT_H
#define VTL_PUBLICATION_TEXT_OP_DISCORD_CONVERT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <VTL/publication/text/discord/VTL_publication_text_op_discord_data.h>
#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/VTL_app_result.h>

VTL_AppResult VTL_discord_ConvertToMarkedText(const VTL_publication_Text* src, const VTL_discord_MarkerList* markers, VTL_publication_MarkedText** out);
VTL_AppResult VTL_discord_ConvertFromMarkedText(const VTL_publication_MarkedText* src, VTL_publication_Text** out);

#ifdef __cplusplus
}
#endif

#endif /* VTL_PUBLICATION_TEXT_OP_DISCORD_CONVERT_H */
