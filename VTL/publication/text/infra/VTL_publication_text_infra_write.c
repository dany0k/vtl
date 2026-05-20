#include <VTL/publication/text/infra/VTL_publication_text_infra_write.h>
#include <stdio.h>

VTL_AppResult VTL_publication_text_Write(VTL_publication_Text* p_text, const VTL_Filename file_name)
{
    if (!p_text || !p_text->text || !file_name) return VTL_res_kErr;
    FILE* f = fopen(file_name, "wb");
    if (!f) return VTL_res_kErr;
    fwrite(p_text->text, 1, p_text->length, f);
    fclose(f);
    return VTL_res_kOk;
}
