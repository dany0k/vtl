#include <VTL/publication/text/asciidoc/VTL_publication_text_op_asciidoc_data.h>

#include <stdlib.h>


// стартовый размер буфера, дальше удваиваем
#define VTL_ASCIIDOC_INITIAL_CAPACITY 64


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
    // удвоение даёт амортизированный O(1) на вставку
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
