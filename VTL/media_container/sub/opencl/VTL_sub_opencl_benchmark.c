/*
 * VTL_sub_opencl_benchmark.c
 *
 * Замер скорости старого и нового подхода к OpenCL.
 * Запускается как отдельный main() — собирается целью VTL_opencl_benchmark.
 *
 * Компиляция через CMake (см. VTL_sub_opencl_benchmark_main.c рядом),
 * или напрямую:
 *
 *   macOS: cc VTL_sub_opencl_benchmark.c VTL_sub_opencl_ctx.c \
 *             -I../../../../.. -framework OpenCL -o benchmark && ./benchmark
 *
 *   Linux: cc VTL_sub_opencl_benchmark.c VTL_sub_opencl_ctx.c \
 *             -I../../../../.. -lOpenCL -o benchmark && ./benchmark
 */

/* Нужен для clock_gettime / CLOCK_MONOTONIC на Linux (POSIX.1-2008) */
#define _POSIX_C_SOURCE 200809L

/* Подавляем предупреждение о целевой версии OpenCL */
#define CL_TARGET_OPENCL_VERSION 120

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif

#include <VTL/media_container/sub/opencl/VTL_sub_opencl_ctx.h>
#include <VTL/media_container/sub/opencl/VTL_sub_opencl_strip_tags.h>
#include <VTL/media_container/sub/opencl/VTL_sub_opencl_kernels.h>
/* VTL_AppResult коды используются как int */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* -------------------------------------------------------------------------
 * Тестовые данные — типичные строки субтитров с ASS-тегами
 * ------------------------------------------------------------------------- */
#define BENCH_LINE_COUNT 500
#define BENCH_REPEATS    10

static const char* BENCH_SAMPLES[] = {
    "{\\b1}Привет, мир!{\\b0}",
    "{\\i1}Это курсив{\\i0}",
    "{\\c&H0000FF&}Синий текст{\\r}",
    "Обычная строка без тегов",
    "{\\pos(320,240)}По центру{\\r}",
    "{\\b1}{\\i1}Жирный курсив{\\i0}{\\b0}",
    "Строка с {\\u1}подчёркиванием{\\u0} внутри",
    "{\\an8}Субтитр сверху",
    "Очень длинная строка субтитра с {\\b1}тегами{\\b0} форматирования",
    "{\\fad(300,300)}Плавное появление и исчезновение",
};
#define BENCH_SAMPLE_COUNT (int)(sizeof(BENCH_SAMPLES) / sizeof(BENCH_SAMPLES[0]))

/* -------------------------------------------------------------------------
 * Утилиты
 * ------------------------------------------------------------------------- */

static double bench_now_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000.0 + t.tv_nsec / 1.0e6;
}

/* -------------------------------------------------------------------------
 * СТАРЫЙ ПОДХОД: полная инициализация OpenCL при каждом вызове.
 * Код намеренно оставлен таким же как в оригинальных файлах проекта.
 * ------------------------------------------------------------------------- */
static int bench_old_strip_tags(const char** in_texts, char** out_texts, size_t count)
{
    cl_int err;
    cl_platform_id platform;
    cl_device_id   device;
    cl_context     context;
    cl_command_queue queue;
    cl_program     program;
    cl_kernel      kernel;

    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) return -1;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_DEFAULT, 1, &device, NULL);
    if (err != CL_SUCCESS) return -1;
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (!context || err != CL_SUCCESS) return -1;
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (!queue || err != CL_SUCCESS) { clReleaseContext(context); return -1; }
    program = clCreateProgramWithSource(context, 1, &VTL_SUB_OPENCL_KERNEL_SOURCE_STRIP_TAGS, NULL, &err);
    if (!program || err != CL_SUCCESS) { clReleaseCommandQueue(queue); clReleaseContext(context); return -1; }
    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) { clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return -1; }
    kernel = clCreateKernel(program, "strip_tags", &err);
    if (!kernel || err != CL_SUCCESS) { clReleaseProgram(program); clReleaseCommandQueue(queue); clReleaseContext(context); return -1; }

    size_t* in_offsets = (size_t*)malloc(count * sizeof(size_t));
    int*    in_lengths = (int*)malloc(count * sizeof(int));
    size_t  total_in   = 0;
    for (size_t i = 0; i < count; ++i) {
        in_offsets[i] = total_in;
        in_lengths[i] = (int)strlen(in_texts[i]);
        total_in += in_lengths[i];
    }
    char* in_data = (char*)malloc(total_in);
    size_t pos = 0;
    for (size_t i = 0; i < count; ++i) { memcpy(in_data + pos, in_texts[i], in_lengths[i]); pos += in_lengths[i]; }

    int*    out_lengths = (int*)malloc(count * sizeof(int));
    size_t* out_offsets = (size_t*)malloc(count * sizeof(size_t));
    size_t  total_out   = 0;
    for (size_t i = 0; i < count; ++i) {
        out_lengths[i] = in_lengths[i];
        out_offsets[i] = total_out;
        total_out += out_lengths[i] + 1;
    }
    char* out_data = (char*)malloc(total_out);

    cl_mem buf_in   = clCreateBuffer(context, CL_MEM_READ_ONLY  | CL_MEM_COPY_HOST_PTR, total_in,          in_data,     &err);
    cl_mem buf_off  = clCreateBuffer(context, CL_MEM_READ_ONLY  | CL_MEM_COPY_HOST_PTR, count*sizeof(int), in_offsets,  &err);
    cl_mem buf_len  = clCreateBuffer(context, CL_MEM_READ_ONLY  | CL_MEM_COPY_HOST_PTR, count*sizeof(int), in_lengths,  &err);
    cl_mem buf_out  = clCreateBuffer(context, CL_MEM_WRITE_ONLY,                         total_out,         NULL,        &err);
    cl_mem buf_ooff = clCreateBuffer(context, CL_MEM_READ_ONLY  | CL_MEM_COPY_HOST_PTR, count*sizeof(int), out_offsets, &err);

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &buf_in);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &buf_off);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &buf_len);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), &buf_out);
    clSetKernelArg(kernel, 4, sizeof(cl_mem), &buf_ooff);

    size_t global = count;
    clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global, NULL, 0, NULL, NULL);
    clFinish(queue);
    clEnqueueReadBuffer(queue, buf_out, CL_TRUE, 0, total_out, out_data, 0, NULL, NULL);

    for (size_t i = 0; i < count; ++i) {
        size_t len = strlen(out_data + out_offsets[i]);
        out_texts[i] = (char*)malloc(len + 1);
        memcpy(out_texts[i], out_data + out_offsets[i], len + 1);
    }

    clReleaseMemObject(buf_in); clReleaseMemObject(buf_off); clReleaseMemObject(buf_len);
    clReleaseMemObject(buf_out); clReleaseMemObject(buf_ooff);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    free(in_offsets); free(in_lengths); free(in_data);
    free(out_lengths); free(out_offsets); free(out_data);
    return VTL_res_kOk;
}

/* -------------------------------------------------------------------------
 * Точка входа бенчмарка
 * ------------------------------------------------------------------------- */
int VTL_opencl_benchmark_run(void)
{
    /* Генерируем BENCH_LINE_COUNT строк по кругу из BENCH_SAMPLES */
    const char** lines = (const char**)malloc(BENCH_LINE_COUNT * sizeof(char*));
    char**       out   = (char**)malloc(BENCH_LINE_COUNT * sizeof(char*));
    for (int i = 0; i < BENCH_LINE_COUNT; ++i)
        lines[i] = BENCH_SAMPLES[i % BENCH_SAMPLE_COUNT];

    printf("\n");
    printf("================================================\n");
    printf(" VTL OpenCL Benchmark — strip_tags\n");
    printf(" Строк: %d  |  Повторов: %d\n", BENCH_LINE_COUNT, BENCH_REPEATS);
    printf("================================================\n\n");

    /* ------------------------------------------------------------------
     * СТАРЫЙ ПОДХОД
     * ------------------------------------------------------------------ */
    printf("--- СТАРЫЙ ПОДХОД (clBuildProgram при каждом вызове) ---\n");
    double old_total = 0;
    for (int r = 0; r < BENCH_REPEATS; ++r) {
        double t0 = bench_now_ms();
        int res = bench_old_strip_tags(lines, out, BENCH_LINE_COUNT);
        double elapsed = bench_now_ms() - t0;
        if (res != 0) { printf("  ОШИБКА: %d\n", res); free(lines); free(out); return 1; }
        old_total += elapsed;
        printf("  вызов %2d: %7.2f мс\n", r + 1, elapsed);
        for (int i = 0; i < BENCH_LINE_COUNT; ++i) { free(out[i]); out[i] = NULL; }
    }
    printf("Итого: %.2f мс  |  Среднее: %.2f мс\n\n", old_total, old_total / BENCH_REPEATS);

    /* ------------------------------------------------------------------
     * НОВЫЙ ПОДХОД — ctx инициализируется один раз
     * ------------------------------------------------------------------ */
    printf("--- НОВЫЙ ПОДХОД (синглтон, clBuildProgram один раз) ---\n");

    VTL_OpenCLContext ctx;
    double t_init0 = bench_now_ms();
    int init_res = (int)VTL_opencl_ctx_init(&ctx);
    double init_ms = bench_now_ms() - t_init0;

    if (init_res != 0) {
        printf("  ctx_init ОШИБКА: %d\n", init_res);
        free(lines); free(out);
        return 1;
    }
    printf("  ctx_init (один раз): %.2f мс\n", init_ms);

    double new_total = 0;
    for (int r = 0; r < BENCH_REPEATS; ++r) {
        double t0 = bench_now_ms();
        int res = (int)VTL_sub_OpenclStripTags(&ctx, lines, out, BENCH_LINE_COUNT);
        double elapsed = bench_now_ms() - t0;
        if (res != 0) { printf("  ОШИБКА: %d\n", res); break; }
        new_total += elapsed;
        printf("  вызов %2d: %7.2f мс\n", r + 1, elapsed);
        for (int i = 0; i < BENCH_LINE_COUNT; ++i) { free(out[i]); out[i] = NULL; }
    }
    printf("Итого вызовов: %.2f мс  |  Среднее: %.2f мс\n", new_total, new_total / BENCH_REPEATS);
    printf("Итого с ctx_init:      %.2f мс\n\n", new_total + init_ms);

    VTL_opencl_ctx_destroy(&ctx);

    /* ------------------------------------------------------------------
     * Итог
     * ------------------------------------------------------------------ */
    double old_avg = old_total / BENCH_REPEATS;
    double new_avg = new_total / BENCH_REPEATS;

    printf("================================================\n");
    printf(" ИТОГ\n");
    printf("================================================\n");
    printf("  Старый — %d вызовов:          %7.2f мс\n", BENCH_REPEATS, old_total);
    printf("  Новый  — %d вызовов + init:   %7.2f мс\n", BENCH_REPEATS, new_total + init_ms);
    printf("  Ускорение (с учётом init):    %.1fx\n\n", old_total / (new_total + init_ms));
    printf("  Средний вызов — старый:  %.2f мс\n", old_avg);
    printf("  Средний вызов — новый:   %.2f мс\n", new_avg);
    printf("  Ускорение вызова:        %.1fx\n", old_avg / new_avg);
    printf("================================================\n\n");

    free(lines);
    free(out);
    return 0;
}
