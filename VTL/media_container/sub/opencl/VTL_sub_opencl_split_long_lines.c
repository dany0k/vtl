#include <VTL/media_container/sub/opencl/VTL_sub_opencl_split_long_lines.h>
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

VTL_AppResult VTL_sub_OpenclSplitLongLines(VTL_OpenCLContext* ctx, const char** in_texts, char**** out_texts, int** out_counts, size_t count, int max_len) {
    cl_int err;
    /* Было: cl_platform_id platform; cl_device_id device; cl_context context;
     *       cl_command_queue queue; cl_program program; cl_kernel kernel;
     * Эти переменные инициализировались заново при каждом вызове функции.
     * Теперь используется ctx, инициализированный один раз. */

    // Подготовка входных данных
    size_t* in_offsets = (size_t*)malloc(count * sizeof(size_t));
    int* in_lengths = (int*)malloc(count * sizeof(int));
    int* max_lens = (int*)malloc(count * sizeof(int));
    size_t total_in = 0;
    for (size_t i = 0; i < count; ++i) {
        in_offsets[i] = total_in;
        in_lengths[i] = (int)strlen(in_texts[i]);
        max_lens[i] = max_len;
        total_in += in_lengths[i];
    }
    char* in_data = (char*)malloc(total_in);
    size_t pos = 0;
    for (size_t i = 0; i < count; ++i) {
        memcpy(in_data + pos, in_texts[i], in_lengths[i]);
        pos += in_lengths[i];
    }

    int* max_splits = (int*)malloc(count * sizeof(int));
    int total_splits = 0;
    for (size_t i = 0; i < count; ++i) {
        max_splits[i] = in_lengths[i] / max_len + 2;
        total_splits += max_splits[i];
    }
    int* out_lengths = (int*)calloc(total_splits, sizeof(int));
    int* out_offsets = (int*)malloc(count * sizeof(int));
    int* out_counts_arr = (int*)calloc(count, sizeof(int));
    int cur_offset = 0;
    for (size_t i = 0; i < count; ++i) {
        out_offsets[i] = cur_offset;
        cur_offset += max_splits[i];
    }

    cl_mem buf_in_data = clCreateBuffer(ctx->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, total_in, in_data, &err);
    cl_mem buf_offsets = clCreateBuffer(ctx->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * sizeof(int), in_offsets, &err);
    cl_mem buf_lengths = clCreateBuffer(ctx->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * sizeof(int), in_lengths, &err);
    cl_mem buf_max_len = clCreateBuffer(ctx->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * sizeof(int), max_lens, &err);
    cl_mem buf_out_offsets = clCreateBuffer(ctx->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * sizeof(int), out_offsets, &err);
    cl_mem buf_out_lengths = clCreateBuffer(ctx->context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, total_splits * sizeof(int), out_lengths, &err);
    cl_mem buf_out_counts = clCreateBuffer(ctx->context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, count * sizeof(int), out_counts_arr, &err);

    clSetKernelArg(ctx->kernel_split_long_lines, 0, sizeof(cl_mem), &buf_in_data);
    clSetKernelArg(ctx->kernel_split_long_lines, 1, sizeof(cl_mem), &buf_offsets);
    clSetKernelArg(ctx->kernel_split_long_lines, 2, sizeof(cl_mem), &buf_lengths);
    clSetKernelArg(ctx->kernel_split_long_lines, 3, sizeof(cl_mem), &buf_max_len);
    clSetKernelArg(ctx->kernel_split_long_lines, 4, sizeof(cl_mem), &buf_out_offsets);
    clSetKernelArg(ctx->kernel_split_long_lines, 5, sizeof(cl_mem), &buf_out_lengths);
    clSetKernelArg(ctx->kernel_split_long_lines, 6, sizeof(cl_mem), &buf_out_counts);

    size_t global = count;
    err = clEnqueueNDRangeKernel(ctx->queue, ctx->kernel_split_long_lines, 1, NULL, &global, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        clReleaseMemObject(buf_in_data);
        clReleaseMemObject(buf_offsets);
        clReleaseMemObject(buf_lengths);
        clReleaseMemObject(buf_max_len);
        clReleaseMemObject(buf_out_offsets);
        clReleaseMemObject(buf_out_lengths);
        clReleaseMemObject(buf_out_counts);
        free(in_offsets); free(in_lengths); free(max_lens); free(in_data); free(max_splits); free(out_lengths); free(out_offsets); free(out_counts_arr);
        return VTL_res_kUnknownError;
    }
    clFinish(ctx->queue);
    err = clEnqueueReadBuffer(ctx->queue, buf_out_lengths, CL_TRUE, 0, total_splits * sizeof(int), out_lengths, 0, NULL, NULL);
    err |= clEnqueueReadBuffer(ctx->queue, buf_out_counts, CL_TRUE, 0, count * sizeof(int), out_counts_arr, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        clReleaseMemObject(buf_in_data);
        clReleaseMemObject(buf_offsets);
        clReleaseMemObject(buf_lengths);
        clReleaseMemObject(buf_max_len);
        clReleaseMemObject(buf_out_offsets);
        clReleaseMemObject(buf_out_lengths);
        clReleaseMemObject(buf_out_counts);
        free(in_offsets); free(in_lengths); free(max_lens); free(in_data); free(max_splits); free(out_lengths); free(out_offsets); free(out_counts_arr);
        return VTL_res_kUnknownError;
    }

    char*** result = (char***)malloc(count * sizeof(char**));
    int* result_counts = (int*)malloc(count * sizeof(int));
    for (size_t i = 0; i < count; ++i) {
        int n = out_counts_arr[i];
        result_counts[i] = n;
        result[i] = (char**)malloc(n * sizeof(char*));
        int offset = out_offsets[i];
        int in_offset = in_offsets[i];
        int pos = 0;
        for (int j = 0; j < n; ++j) {
            int len = out_lengths[offset + j];
            result[i][j] = (char*)malloc(len + 1);
            memcpy(result[i][j], in_texts[i] + pos, len);
            result[i][j][len] = '\0';
            pos += len;
            while (in_texts[i][pos] == ' ') pos++;
        }
    }
    *out_texts = result;
    *out_counts = result_counts;

    clReleaseMemObject(buf_in_data);
    clReleaseMemObject(buf_offsets);
    clReleaseMemObject(buf_lengths);
    clReleaseMemObject(buf_max_len);
    clReleaseMemObject(buf_out_offsets);
    clReleaseMemObject(buf_out_lengths);
    clReleaseMemObject(buf_out_counts);
    free(in_offsets); free(in_lengths); free(max_lens); free(in_data); free(max_splits); free(out_lengths); free(out_offsets); free(out_counts_arr);
    return VTL_res_kOk;
} 