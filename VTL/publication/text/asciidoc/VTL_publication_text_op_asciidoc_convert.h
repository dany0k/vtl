#ifndef _VTL_PUBLICATION_TEXT_OP_ASCIIDOC_CONVERT_H
#define _VTL_PUBLICATION_TEXT_OP_ASCIIDOC_CONVERT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/publication/text/asciidoc/VTL_publication_text_op_asciidoc_data.h>
#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/VTL_app_result.h>
#include <stddef.h>


// сортирует маркеры по позиции в тексте — чтобы конвертация шла одним проходом слева направо
VTL_AppResult VTL_asciidoc_SortMarkers(VTL_asciidoc_MarkerList* list);

// сливает N частичных списков (от каждого сканера) в один и сортирует
VTL_AppResult VTL_asciidoc_MergeMarkers(const VTL_asciidoc_MarkerList* parts,
                                        size_t parts_count,
                                        VTL_asciidoc_MarkerList* out);

// проходит по отсортированным маркерам и нарезает текст на части с флагами BOLD/ITALIC/STRIKE
// сами токены разметки выкидываются из выходного текста
VTL_AppResult VTL_asciidoc_ConvertToMarkedText(const VTL_publication_Text* src,
                                                const VTL_asciidoc_MarkerList* markers,
                                                VTL_publication_MarkedText** out);


#ifdef __cplusplus
}
#endif

#endif
