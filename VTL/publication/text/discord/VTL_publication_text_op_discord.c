#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#endif
#endif

#include <VTL/publication/text/discord/VTL_publication_text_op_discord.h>

VTL_AppResult VTL_discord_ParseTextSequential(const VTL_publication_Text *src, VTL_publication_MarkedText **out) {
    if (!src || !out) {
        return VTL_res_kInvalidParamErr;
    }

    VTL_discord_MarkerList markers;
    VTL_discord_MarkerListInit(&markers);

    VTL_AppResult res = VTL_discord_ParseDocumentSequential(src, &markers);
    if (res == VTL_res_kOk) {
        res = VTL_discord_ConvertToMarkedText(src, &markers, out);
    }

    VTL_discord_MarkerListFree(&markers);
    return res;
}

VTL_AppResult VTL_discord_ParseTextParallel(const VTL_publication_Text *src, VTL_publication_MarkedText **out) {
    if (!src || !out) {
        return VTL_res_kInvalidParamErr;
    }

    VTL_discord_MarkerList markers;
    VTL_discord_MarkerListInit(&markers);

    VTL_AppResult res = VTL_discord_ParseDocumentParallel(src, &markers);
    if (res == VTL_res_kOk) {
        res = VTL_discord_ConvertToMarkedText(src, &markers, out);
    }

    VTL_discord_MarkerListFree(&markers);
    return res;
}

VTL_AppResult VTL_discord_SerializeToText(const VTL_publication_MarkedText *src, VTL_publication_Text **out) {
    return VTL_discord_ConvertFromMarkedText(src, out);
}
