#include <VTL/utils/VTL_utils_file.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

VTL_AppResult VTL_file_OpenForReading(VTL_File** pp_file, VTL_Filename file_name)
{
    if (!pp_file || !file_name) return VTL_res_kErr;
    *pp_file = fopen(file_name, "rb");
    return (*pp_file) ? VTL_res_kOk : VTL_res_kErr;
}

VTL_AppResult VTL_file_OpenForWriting(VTL_File** pp_file, VTL_Filename file_name)
{
    if (!pp_file || !file_name) return VTL_res_kErr;
    *pp_file = fopen(file_name, "wb");
    return (*pp_file) ? VTL_res_kOk : VTL_res_kErr;
}

VTL_AppResult VTL_file_ReadRawData(VTL_BufferData** buffer_data, const VTL_Filename file_name)
{
    if (!buffer_data || !file_name) return VTL_res_kErr;
    FILE* f = fopen(file_name, "rb");
    if (!f) return VTL_res_kErr;

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return VTL_res_kErr; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return VTL_res_kErr; }
    rewind(f);

    VTL_BufferData* buf = (VTL_BufferData*)malloc(sizeof(VTL_BufferData));
    if (!buf) { fclose(f); return VTL_res_kErr; }
    buf->data = (char*)malloc((size_t)sz + 1);
    if (!buf->data) { free(buf); fclose(f); return VTL_res_kErr; }
    size_t got = fread(buf->data, 1, (size_t)sz, f);
    fclose(f);
    buf->data[got] = '\0';
    buf->data_size = got;
    *buffer_data = buf;
    return VTL_res_kOk;
}

VTL_AppResult VTL_file_WriteRawData(VTL_BufferData** buffer_data, const VTL_Filename file_name)
{
    if (!buffer_data || !*buffer_data || !file_name) return VTL_res_kErr;
    FILE* f = fopen(file_name, "wb");
    if (!f) return VTL_res_kErr;
    size_t wrote = fwrite((*buffer_data)->data, 1, (*buffer_data)->data_size, f);
    fclose(f);
    return (wrote == (*buffer_data)->data_size) ? VTL_res_kOk : VTL_res_kErr;
}

VTL_AppResult VTL_file_Copy(const VTL_Filename out_file_name, const VTL_Filename src_file_name)
{
    if (!out_file_name || !src_file_name) return VTL_res_kErr;
    FILE* in = fopen(src_file_name, "rb");
    if (!in) return VTL_res_kErr;
    FILE* out = fopen(out_file_name, "wb");
    if (!out) { fclose(in); return VTL_res_kErr; }

    char buf[8192];
    size_t n;
    VTL_AppResult res = VTL_res_kOk;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) { res = VTL_res_kErr; break; }
    }
    fclose(in);
    fclose(out);
    return res;
}

bool VTL_file_CheckEquality(const VTL_Filename first_file_name, const VTL_Filename second_file_name)
{
    if (!first_file_name || !second_file_name) return false;
    return strcmp(first_file_name, second_file_name) == 0;
}
