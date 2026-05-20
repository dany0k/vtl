#ifndef _VTL_PUBLICATION_TEXT_OP_ASCIIDOC_LOAD_H
#define _VTL_PUBLICATION_TEXT_OP_ASCIIDOC_LOAD_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <VTL/publication/text/VTL_publication_text_data.h>
#include <VTL/VTL_app_result.h>
#include <stddef.h>


// читает файл и парсит через параллельную версию
VTL_AppResult VTL_asciidoc_ParseFile(const char* path,
                                      VTL_publication_MarkedText** out);

// batch: N файлов сразу
// Sequential — по очереди в одном потоке
// Parallel — поток на файл (внутри файла sequential, чтобы не делать N*15 потоков)
VTL_AppResult VTL_asciidoc_ParseFileBatchSequential(const char* const* paths,
                                                     size_t paths_count,
                                                     VTL_publication_MarkedText** out_marked);

VTL_AppResult VTL_asciidoc_ParseFileBatchParallel(const char* const* paths,
                                                   size_t paths_count,
                                                   VTL_publication_MarkedText** out_marked);


#ifdef __cplusplus
}
#endif

#endif
