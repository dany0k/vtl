#include <VTL/media_container/sub/opencl/VTL_sub_opencl_format_time.h>
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif
#include <VTL/media_container/sub/opencl/VTL_sub_opencl_kernels.h>
#include <VTL/media_container/sub/opencl/VTL_sub_opencl_ctx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

VTL_AppResult VTL_sub_OpenclFormatTime(VTL_OpenCLContext* ctx, const double* in_times, char*** out_texts, size_t count, int format) {
    cl_int err;
    /* Было: cl_platform_id platform; cl_device_id device; cl_context context;
     *       cl_command_queue queue; cl_program program; cl_kernel kernel;
     * Эти переменные инициализировались заново при каждом вызове функции.
     * Теперь используется ctx, инициализированный один раз. */

    // Максимальная длина строки: SRT/VTT = 12, ASS = 11 (+1 нуль-терминатор)
    int max_len = 16;
    int* out_offsets = (int*)malloc(count * sizeof(int));
    for (size_t i = 0; i < count; ++i) out_offsets[i] = (int)(i * max_len);
    char* out_data = (char*)malloc(count * max_len);

    cl_mem buf_in_times = clCreateBuffer(ctx->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * sizeof(double), (void*)in_times, &err);
    cl_mem buf_out_data = clCreateBuffer(ctx->context, CL_MEM_WRITE_ONLY, count * max_len, NULL, &err);
    cl_mem buf_out_offsets = clCreateBuffer(ctx->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * sizeof(int), out_offsets, &err);

    clSetKernelArg(ctx->kernel_format_time, 0, sizeof(cl_mem), &buf_in_times);
    clSetKernelArg(ctx->kernel_format_time, 1, sizeof(cl_mem), &buf_out_data);
    clSetKernelArg(ctx->kernel_format_time, 2, sizeof(cl_mem), &buf_out_offsets);
    clSetKernelArg(ctx->kernel_format_time, 3, sizeof(int), &format);

    size_t global = count;
    err = clEnqueueNDRangeKernel(ctx->queue, ctx->kernel_format_time, 1, NULL, &global, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        clReleaseMemObject(buf_in_times);
        clReleaseMemObject(buf_out_data);
        clReleaseMemObject(buf_out_offsets);
        free(out_offsets); free(out_data);
        return VTL_res_kUnknownError;
    }
    clFinish(ctx->queue);

    char** texts = (char**)malloc(count * sizeof(char*));
    err = clEnqueueReadBuffer(ctx->queue, buf_out_data, CL_TRUE, 0, count * max_len, out_data, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        clReleaseMemObject(buf_in_times);
        clReleaseMemObject(buf_out_data);
        clReleaseMemObject(buf_out_offsets);
        free(out_offsets); free(out_data); free(texts);
        return VTL_res_kUnknownError;
    }
    for (size_t i = 0; i < count; ++i) {
        texts[i] = (char*)malloc(max_len);
        memcpy(texts[i], out_data + i * max_len, max_len);
        texts[i][max_len-1] = '\0';
    }
    *out_texts = texts;

    clReleaseMemObject(buf_in_times);
    clReleaseMemObject(buf_out_data);
    clReleaseMemObject(buf_out_offsets);
    free(out_offsets); free(out_data);
    return VTL_res_kOk;
} 