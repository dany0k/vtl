#include <VTL/media_container/sub/opencl/VTL_sub_opencl_detect_encoding.h>
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

VTL_AppResult VTL_sub_OpenclDetectEncoding(VTL_OpenCLContext* ctx, const char** in_texts, int** out_encodings, size_t count) {
    cl_int err;
    /* Было: cl_platform_id platform; cl_device_id device; cl_context context;
     *       cl_command_queue queue; cl_program program; cl_kernel kernel;
     * Эти переменные инициализировались заново при каждом вызове функции.
     * Теперь используется ctx, инициализированный один раз. */

    size_t* in_offsets = (size_t*)malloc(count * sizeof(size_t));
    int* in_lengths = (int*)malloc(count * sizeof(int));
    size_t total_in = 0;
    for (size_t i = 0; i < count; ++i) {
        in_offsets[i] = total_in;
        in_lengths[i] = (int)strlen(in_texts[i]);
        total_in += in_lengths[i];
    }
    char* in_data = (char*)malloc(total_in);
    size_t pos = 0;
    for (size_t i = 0; i < count; ++i) {
        memcpy(in_data + pos, in_texts[i], in_lengths[i]);
        pos += in_lengths[i];
    }
    int* encodings = (int*)malloc(count * sizeof(int));

    cl_mem buf_in_data = clCreateBuffer(ctx->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, total_in, in_data, &err);
    cl_mem buf_offsets = clCreateBuffer(ctx->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * sizeof(int), in_offsets, &err);
    cl_mem buf_lengths = clCreateBuffer(ctx->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * sizeof(int), in_lengths, &err);
    cl_mem buf_out_encodings = clCreateBuffer(ctx->context, CL_MEM_WRITE_ONLY, count * sizeof(int), NULL, &err);

    clSetKernelArg(ctx->kernel_detect_encoding, 0, sizeof(cl_mem), &buf_in_data);
    clSetKernelArg(ctx->kernel_detect_encoding, 1, sizeof(cl_mem), &buf_offsets);
    clSetKernelArg(ctx->kernel_detect_encoding, 2, sizeof(cl_mem), &buf_lengths);
    clSetKernelArg(ctx->kernel_detect_encoding, 3, sizeof(cl_mem), &buf_out_encodings);

    size_t global = count;
    err = clEnqueueNDRangeKernel(ctx->queue, ctx->kernel_detect_encoding, 1, NULL, &global, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        clReleaseMemObject(buf_in_data);
        clReleaseMemObject(buf_offsets);
        clReleaseMemObject(buf_lengths);
        clReleaseMemObject(buf_out_encodings);
        free(in_offsets); free(in_lengths); free(in_data); free(encodings);
        return VTL_res_kUnknownError;
    }
    clFinish(ctx->queue);
    err = clEnqueueReadBuffer(ctx->queue, buf_out_encodings, CL_TRUE, 0, count * sizeof(int), encodings, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        clReleaseMemObject(buf_in_data);
        clReleaseMemObject(buf_offsets);
        clReleaseMemObject(buf_lengths);
        clReleaseMemObject(buf_out_encodings);
        free(in_offsets); free(in_lengths); free(in_data); free(encodings);
        return VTL_res_kUnknownError;
    }
    *out_encodings = encodings;

    clReleaseMemObject(buf_in_data);
    clReleaseMemObject(buf_offsets);
    clReleaseMemObject(buf_lengths);
    clReleaseMemObject(buf_out_encodings);
    free(in_offsets); free(in_lengths); free(in_data);
    return VTL_res_kOk;
} 