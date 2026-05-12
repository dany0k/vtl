#include <VTL/publication/text/asciidoc/VTL_publication_text_op_asciidoc_convert.h>
#include <VTL/VTL_publication_markup_text_flags.h>

#include <stdlib.h>


// сортировка для qsort: сначала по позиции, при равной — по типу
static int VTL_asciidoc_marker_cmp(const void* a, const void* b)
{
    const VTL_asciidoc_Marker* ma = (const VTL_asciidoc_Marker*)a;
    const VTL_asciidoc_Marker* mb = (const VTL_asciidoc_Marker*)b;
    if (ma->pos < mb->pos) return -1;
    if (ma->pos > mb->pos) return  1;
    if (ma->kind < mb->kind) return -1;
    if (ma->kind > mb->kind) return  1;
    return 0;
}

VTL_AppResult VTL_asciidoc_SortMarkers(VTL_asciidoc_MarkerList* list)
{
    if (!list) return VTL_res_kInvalidParamErr;
    if (list->length > 1) {
        qsort(list->items, list->length, sizeof(VTL_asciidoc_Marker),
              VTL_asciidoc_marker_cmp);
    }
    return VTL_res_kOk;
}

VTL_AppResult VTL_asciidoc_MergeMarkers(const VTL_asciidoc_MarkerList* parts,
                                        size_t parts_count,
                                        VTL_asciidoc_MarkerList* out)
{
    if (!parts || !out) return VTL_res_kInvalidParamErr;

    // считаем общий размер заранее — чтобы выделить память одним reserve, без realloc'ов
    size_t total = 0;
    for (size_t i = 0; i < parts_count; ++i) {
        total += parts[i].length;
    }
    VTL_AppResult res = VTL_asciidoc_MarkerListReserve(out, total);
    if (res != VTL_res_kOk) return res;

    for (size_t i = 0; i < parts_count; ++i) {
        for (size_t j = 0; j < parts[i].length; ++j) {
            res = VTL_asciidoc_MarkerListPush(out, parts[i].items[j]);
            if (res != VTL_res_kOk) return res;
        }
    }
    return VTL_asciidoc_SortMarkers(out);
}


// собирает одну часть для MarkedText
static VTL_publication_marked_text_Part VTL_asciidoc_make_part(const char* text_start,
    size_t length, VTL_publication_text_modification_Flags flags)
{
    VTL_publication_marked_text_Part p;
    p.text = (VTL_publication_text_Symbol*)text_start;
    p.length = length;
    p.type = flags;
    return p;
}

VTL_AppResult VTL_asciidoc_ConvertToMarkedText(const VTL_publication_Text* src,
                                                const VTL_asciidoc_MarkerList* markers,
                                                VTL_publication_MarkedText** out)
{
    if (!src || !src->text || !markers || !out) return VTL_res_kInvalidParamErr;

    VTL_publication_MarkedText* block =
        (VTL_publication_MarkedText*)malloc(sizeof(VTL_publication_MarkedText));
    if (!block) return VTL_res_kMemAllocErr;

    // между парами маркеров — кусок текста, плюс пролог и хвост
    size_t max_parts = markers->length * 2 + 2;
    block->parts = (VTL_publication_marked_text_Part*)malloc(
        max_parts * sizeof(VTL_publication_marked_text_Part));
    if (!block->parts) { free(block); return VTL_res_kMemAllocErr; }
    block->length = 0;

    VTL_publication_text_modification_Flags flags = 0;
    size_t cursor = 0;  // докуда уже записали

    for (size_t i = 0; i < markers->length; ++i) {
        const VTL_asciidoc_Marker* m = &markers->items[i];

        // маркер внутри уже обработанного куска — пропускаем (вложенные конструкции)
        if (m->pos < cursor) continue;

        // кусок обычного текста перед маркером
        if (m->pos > cursor) {
            block->parts[block->length++] = VTL_asciidoc_make_part(
                src->text + cursor, m->pos - cursor, flags);
        }

        // меняем флаги в зависимости от типа маркера. остальные просто скипаются
        switch (m->kind) {
            case VTL_asciidoc_marker_kBoldStart:    flags |=  VTL_TEXT_MODIFICATION_BOLD;          break;
            case VTL_asciidoc_marker_kBoldEnd:      flags &= ~VTL_TEXT_MODIFICATION_BOLD;          break;
            case VTL_asciidoc_marker_kItalicStart:  flags |=  VTL_TEXT_MODIFICATION_ITALIC;        break;
            case VTL_asciidoc_marker_kItalicEnd:    flags &= ~VTL_TEXT_MODIFICATION_ITALIC;        break;
            case VTL_asciidoc_marker_kStrikeStart:  flags |=  VTL_TEXT_MODIFICATION_STRIKETHROUGH; break;
            case VTL_asciidoc_marker_kStrikeEnd:    flags &= ~VTL_TEXT_MODIFICATION_STRIKETHROUGH; break;

            // у этих типов нет inline-флага во внутреннем формате — просто едим токен
            default: break;
        }

        cursor = m->pos + m->length;
    }

    // хвост после последнего маркера
    if (cursor < src->length) {
        block->parts[block->length++] = VTL_asciidoc_make_part(
            src->text + cursor, src->length - cursor, flags);
    }

    *out = block;
    return VTL_res_kOk;
}
