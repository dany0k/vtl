#include <VTL/media_container/sub/opencl/VTL_sub_opencl_ctx.h>
#include <VTL/media_container/sub/opencl/VTL_sub_opencl_kernels.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Вспомогательная функция: компилирует одно ядро из исходника и возвращает cl_kernel.
 * Программа (cl_program) освобождается сразу после создания ядра — она больше не нужна. */
static cl_kernel vtl_build_kernel(cl_context ctx, cl_device_id device, const char *src, const char *name, cl_int *err) {
    cl_program prog = clCreateProgramWithSource(ctx, 1, &src, NULL, err);
    if (!prog || *err != CL_SUCCESS) {
        return NULL;
    }

    *err = clBuildProgram(prog, 1, &device, NULL, NULL, NULL);
    if (*err != CL_SUCCESS) {
        /* Выводим лог компиляции для диагностики ошибок в kernel-коде */
        size_t log_size;
        clGetProgramBuildInfo(prog, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char *log = (char *) malloc(log_size + 1);
        if (log) {
            clGetProgramBuildInfo(prog, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
            log[log_size] = '\0';
            fprintf(stderr, "[VTL OpenCL] Build error '%s':\n%s\n", name, log);
            free(log);
        }
        clReleaseProgram(prog);
        return NULL;
    }

    cl_kernel k = clCreateKernel(prog, name, err);
    clReleaseProgram(prog);
    return (!k || *err != CL_SUCCESS) ? NULL : k;
}

VTL_AppResult VTL_opencl_ctx_init(VTL_OpenCLContext *ctx) {
    if (!ctx) {
        return VTL_res_kArgumentError;
    }
    memset(ctx, 0, sizeof(*ctx));

    cl_int err;

    err = clGetPlatformIDs(1, &ctx->platform, NULL);
    if (err != CL_SUCCESS) {
        return VTL_res_kUnknownError;
    }

    err = clGetDeviceIDs(ctx->platform, CL_DEVICE_TYPE_DEFAULT, 1, &ctx->device, NULL);
    if (err != CL_SUCCESS) {
        return VTL_res_kUnknownError;
    }

    ctx->context = clCreateContext(NULL, 1, &ctx->device, NULL, NULL, &err);
    if (!ctx->context || err != CL_SUCCESS) {
        return VTL_res_kUnknownError;
    }

    ctx->queue = clCreateCommandQueue(ctx->context, ctx->device, 0, &err);
    if (!ctx->queue || err != CL_SUCCESS) {
        clReleaseContext(ctx->context);
        return VTL_res_kUnknownError;
    }

        /* Компилируем все ядра один раз. В оригинале это делалось внутри каждой функции
         * при каждом вызове — clBuildProgram на каждый вызов StripTags/CleanTags/и т.д. */
#define BUILD_OR_FAIL(field, src, name)                                            \
    ctx->field = vtl_build_kernel(ctx->context, ctx->device, src, name, &err);    \
    if (!ctx->field || err != CL_SUCCESS) { VTL_opencl_ctx_destroy(ctx); return VTL_res_kUnknownError; }

    BUILD_OR_FAIL(kernel_strip_tags, VTL_SUB_OPENCL_KERNEL_SOURCE_STRIP_TAGS, "strip_tags")
    BUILD_OR_FAIL(kernel_clean_tags, VTL_SUB_OPENCL_KERNEL_SOURCE_CLEAN_TAGS, "clean_tags")
    BUILD_OR_FAIL(kernel_detect_encoding, VTL_SUB_OPENCL_KERNEL_SOURCE_DETECT_ENCODING, "detect_encoding")
    BUILD_OR_FAIL(kernel_split_long_lines, VTL_SUB_OPENCL_KERNEL_SOURCE_SPLIT_LONG_LINES, "split_long_lines")
    BUILD_OR_FAIL(kernel_apply_ass_style, VTL_SUB_OPENCL_KERNEL_SOURCE_APPLY_ASS_STYLE, "apply_ass_style")
    BUILD_OR_FAIL(kernel_string_stats, VTL_SUB_OPENCL_KERNEL_SOURCE_STRING_STATS, "string_stats")
    BUILD_OR_FAIL(kernel_hash_strings, VTL_SUB_OPENCL_KERNEL_SOURCE_HASH_STRINGS, "hash_strings")
    BUILD_OR_FAIL(kernel_squeeze_repeats, VTL_SUB_OPENCL_KERNEL_SOURCE_SQUEEZE_REPEATS, "squeeze_repeats")
    BUILD_OR_FAIL(kernel_format_numbers, VTL_SUB_OPENCL_KERNEL_SOURCE_FORMAT_NUMBERS, "format_numbers")
    BUILD_OR_FAIL(kernel_format_time, VTL_SUB_OPENCL_KERNEL_SOURCE_FORMAT_TIME, "format_time")

#undef BUILD_OR_FAIL

    ctx->initialized = 1;
    return VTL_res_kOk;
}

void VTL_opencl_ctx_destroy(VTL_OpenCLContext *ctx) {
    if (!ctx) {
        return;
    }
#define REL(f) if (ctx->f) { clReleaseKernel(ctx->f); ctx->f = NULL; }
    REL(kernel_strip_tags)
    REL(kernel_clean_tags)
    REL(kernel_detect_encoding)
    REL(kernel_split_long_lines)
    REL(kernel_apply_ass_style)
    REL(kernel_string_stats)
    REL(kernel_hash_strings)
    REL(kernel_squeeze_repeats)
    REL(kernel_format_numbers)
    REL(kernel_format_time)
#undef REL
    if (ctx->queue) {
        clReleaseCommandQueue(ctx->queue);
        ctx->queue = NULL;
    }
    if (ctx->context) {
        clReleaseContext(ctx->context);
        ctx->context = NULL;
    }
    ctx->initialized = 0;
}
