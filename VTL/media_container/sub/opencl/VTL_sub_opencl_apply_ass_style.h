#ifndef VTL_SUB_OPENCL_APPLY_ASS_STYLE_H
#define VTL_SUB_OPENCL_APPLY_ASS_STYLE_H

#include <stddef.h>
#include <VTL/VTL_app_result.h>
#include <VTL/media_container/sub/opencl/VTL_sub_opencl_ctx.h>

#ifdef __cplusplus
extern "C" {
#endif

// Массовое применение или удаление ASS-стилей к строкам через OpenCL
// in_texts: массив указателей на строки (вход)
// out_texts: массив строк (char*), выделяется внутри функции, освобождать вызывающему
// count: количество строк
// style_tag: строка-шаблон ASS-тега (например, "{\\b1}...{\\b0}")
// mode: 0 = удалить стиль, 1 = добавить стиль
// Возвращает 0 при успехе, иначе код ошибки
VTL_AppResult VTL_sub_OpenclApplyAssStyle(VTL_OpenCLContext* ctx, const char** in_texts, char*** out_texts, size_t count, const char* style_tag, int mode);

#ifdef __cplusplus
}
#endif

#endif // VTL_SUB_OPENCL_APPLY_ASS_STYLE_H 