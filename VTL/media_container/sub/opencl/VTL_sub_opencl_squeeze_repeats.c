#include <VTL/media_container/sub/opencl/VTL_sub_opencl_squeeze_repeats.h>
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

VTL_AppResult VTL_sub_OpenclSqueezeRepeats(VTL_OpenCLContext* ctx, const char** in_texts, char*** out_texts, size_t count) {
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

    int* out_lengths = (int*)malloc(count * sizeof(int));
    for (size_t i = 0; i < count; ++i) out_lengths[i] = in_lengths[i] + 1;
    size_t* out_offsets = (size_t*)malloc(count * sizeof(size_t));
    size_t total_out = 0;
    for (size_t i = 0; i < count; ++i) {
        out_offsets[i] = total_out;
        total_out += out_lengths[i];
    }
    char* out_data = (char*)malloc(total_out);

    cl_mem buf_in_data = clCreateBuffer(ctx->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, total_in, in_data, &err);
    cl_mem buf_offsets = clCreateBuffer(ctx->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * sizeof(int), in_offsets, &err);
    cl_mem buf_lengths = clCreateBuffer(ctx->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * sizeof(int), in_lengths, &err);
    cl_mem buf_out_data = clCreateBuffer(ctx->context, CL_MEM_WRITE_ONLY, total_out, NULL, &err);
    cl_mem buf_out_offsets = clCreateBuffer(ctx->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * sizeof(int), out_offsets, &err);

    clSetKernelArg(ctx->kernel_squeeze_repeats, 0, sizeof(cl_mem), &buf_in_data);
    clSetKernelArg(ctx->kernel_squeeze_repeats, 1, sizeof(cl_mem), &buf_offsets);
    clSetKernelArg(ctx->kernel_squeeze_repeats, 2, sizeof(cl_mem), &buf_lengths);
    clSetKernelArg(ctx->kernel_squeeze_repeats, 3, sizeof(cl_mem), &buf_out_data);
    clSetKernelArg(ctx->kernel_squeeze_repeats, 4, sizeof(cl_mem), &buf_out_offsets);

    size_t global = count;
    err = clEnqueueNDRangeKernel(ctx->queue, ctx->kernel_squeeze_repeats, 1, NULL, &global, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        clReleaseMemObject(buf_in_data);
        clReleaseMemObject(buf_offsets);
        clReleaseMemObject(buf_lengths);
        clReleaseMemObject(buf_out_data);
        clReleaseMemObject(buf_out_offsets);
        free(in_offsets); free(in_lengths); free(in_data); free(out_lengths); free(out_offsets); free(out_data);
        return VTL_res_kUnknownError;
    }
    clFinish(ctx->queue);
    char** texts = (char**)malloc(count * sizeof(char*));
    err = clEnqueueReadBuffer(ctx->queue, buf_out_data, CL_TRUE, 0, total_out, out_data, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        clReleaseMemObject(buf_in_data);
        clReleaseMemObject(buf_offsets);
        clReleaseMemObject(buf_lengths);
        clReleaseMemObject(buf_out_data);
        clReleaseMemObject(buf_out_offsets);
        free(in_offsets); free(in_lengths); free(in_data); free(out_lengths); free(out_offsets); free(out_data); free(texts);
        return VTL_res_kUnknownError;
    }
    for (size_t i = 0; i < count; ++i) {
        texts[i] = (char*)malloc(out_lengths[i]);
        memcpy(texts[i], out_data + out_offsets[i], out_lengths[i]);
        texts[i][out_lengths[i]-1] = '\0';
    }
    *out_texts = texts;

    clReleaseMemObject(buf_in_data);
    clReleaseMemObject(buf_offsets);
    clReleaseMemObject(buf_lengths);
    clReleaseMemObject(buf_out_data);
    clReleaseMemObject(buf_out_offsets);
    free(in_offsets); free(in_lengths); free(in_data); free(out_lengths); free(out_offsets); free(out_data);
    return VTL_res_kOk;
} 