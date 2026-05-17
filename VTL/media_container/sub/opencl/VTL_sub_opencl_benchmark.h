#ifndef VTL_SUB_OPENCL_BENCHMARK_H
#define VTL_SUB_OPENCL_BENCHMARK_H

#ifdef __cplusplus
extern "C" {
#endif

/* Запускает замер скорости старого и нового подхода, выводит результат в stdout.
 * Возвращает 0 при успехе, 1 при ошибке OpenCL. */
int VTL_opencl_benchmark_run(void);

#ifdef __cplusplus
}
#endif

#endif /* VTL_SUB_OPENCL_BENCHMARK_H */
