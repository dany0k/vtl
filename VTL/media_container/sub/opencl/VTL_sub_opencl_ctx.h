#ifndef VTL_SUB_OPENCL_CTX_H
#define VTL_SUB_OPENCL_CTX_H

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else

#include <CL/opencl.h>

#endif

#include <VTL/VTL_app_result.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Синглтон OpenCL-контекста.
 *
 * В оригинале каждая функция (StripTags, CleanTags, ...) при каждом вызове
 * заново выполняла полный цикл инициализации:
 *   clGetPlatformIDs -> clGetDeviceIDs -> clCreateContext -> clCreateCommandQueue
 *   -> clCreateProgramWithSource -> clBuildProgram -> clCreateKernel
 *
 * clBuildProgram — это JIT-компиляция GPU-шейдера, занимает десятки мс.
 * При последовательном вызове нескольких функций накладные расходы суммируются.
 *
 * Теперь: инициализируется один раз через VTL_opencl_ctx_init(),
 * передаётся во все функции, уничтожается через VTL_opencl_ctx_destroy().
 */
typedef struct {
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;

    /* Скомпилированные ядра — создаются один раз в VTL_opencl_ctx_init() */
    cl_kernel kernel_strip_tags;
    cl_kernel kernel_clean_tags;
    cl_kernel kernel_detect_encoding;
    cl_kernel kernel_split_long_lines;
    cl_kernel kernel_apply_ass_style;
    cl_kernel kernel_string_stats;
    cl_kernel kernel_hash_strings;
    cl_kernel kernel_squeeze_repeats;
    cl_kernel kernel_format_numbers;
    cl_kernel kernel_format_time;

    int initialized;
} VTL_OpenCLContext;

VTL_AppResult VTL_opencl_ctx_init(VTL_OpenCLContext *ctx);

void VTL_opencl_ctx_destroy(VTL_OpenCLContext *ctx);

#ifdef __cplusplus
}
#endif

#endif /* VTL_SUB_OPENCL_CTX_H */
