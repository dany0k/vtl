#include <VTL/publication/text/infra/VTL_publication_text_infra_read.h>
#include <stdlib.h>
#include <stdio.h>

VTL_AppResult VTL_publication_text_Read(VTL_publication_Text** pp_text, const VTL_Filename file_name)
{
    if (!pp_text || !file_name) return VTL_res_kErr;
    FILE* f = fopen(file_name, "rb");
    if (!f) return VTL_res_kErr;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) { fclose(f); return VTL_res_kErr; }
    *pp_text = (VTL_publication_Text*)calloc(1, sizeof(VTL_publication_Text));
    if (!*pp_text) { fclose(f); return VTL_res_kErr; }
    (*pp_text)->text = (VTL_publication_text_Symbol*)malloc((size_t)sz + 1);
    if (!(*pp_text)->text) { free(*pp_text); fclose(f); return VTL_res_kErr; }
    fread((*pp_text)->text, 1, (size_t)sz, f);
    (*pp_text)->text[sz] = 0;
    (*pp_text)->length = (size_t)sz;
    fclose(f);
    return VTL_res_kOk;
}
