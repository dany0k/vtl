#ifndef VTL_SUB_OPENCL_CLEAN_TAGS_H
#define VTL_SUB_OPENCL_CLEAN_TAGS_H

#include <stddef.h>
#include <VTL/VTL_app_result.h>
#include <VTL/media_container/sub/opencl/VTL_sub_opencl_ctx.h>

#ifdef __cplusplus
extern "C" {
#endif

// Массовая очистка строк от тегов, спецсимволов и эмодзи с помощью OpenCL
VTL_AppResult VTL_sub_OpenclCleanTags(VTL_OpenCLContext* ctx, const char** in_texts, char** out_texts, size_t count);

#ifdef __cplusplus
}
#endif

#endif // VTL_SUB_OPENCL_CLEAN_TAGS_H 