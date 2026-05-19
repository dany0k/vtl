#include <VTL/publication/text/discord/VTL_publication_text_op_discord_data.h>
#include <stdlib.h>
#include <string.h>

#define VTL_DISCORD_LIST_INITIAL_CAPACITY 32

VTL_AppResult VTL_discord_MarkerListInit(VTL_discord_MarkerList *list) {
    if (!list) {
        return VTL_res_kInvalidParamErr;
    }
    list->items = NULL;
    list->length = 0;
    list->capacity = 0;
    return VTL_res_kOk;
}

VTL_AppResult VTL_discord_MarkerListReserve(VTL_discord_MarkerList *list, size_t capacity) {
    if (!list) {
        return VTL_res_kInvalidParamErr;
    }
    if (capacity <= list->capacity) {
        return VTL_res_kOk;
    }

    VTL_discord_Marker *p = (VTL_discord_Marker *) realloc(list->items, capacity * sizeof(VTL_discord_Marker));
    if (!p) {
        return VTL_res_kMemAllocErr;
    }

    list->items = p;
    list->capacity = capacity;
    return VTL_res_kOk;
}

VTL_AppResult VTL_discord_MarkerListPush(VTL_discord_MarkerList *list, VTL_discord_Marker marker) {
    if (!list) {
        return VTL_res_kInvalidParamErr;
    }

    if (list->length == list->capacity) {
        size_t new_cap = list->capacity == 0 ? VTL_DISCORD_LIST_INITIAL_CAPACITY : list->capacity * 2;
        VTL_AppResult res = VTL_discord_MarkerListReserve(list, new_cap);
        if (res != VTL_res_kOk) {
            return res;
        }
    }

    list->items[list->length++] = marker;
    return VTL_res_kOk;
}

void VTL_discord_MarkerListFree(VTL_discord_MarkerList *list) {
    if (!list) {
        return;
    }
    free(list->items);
    list->items = NULL;
    list->length = 0;
    list->capacity = 0;
}
