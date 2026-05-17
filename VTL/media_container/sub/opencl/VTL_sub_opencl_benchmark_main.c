#include <VTL/media_container/sub/opencl/VTL_sub_opencl_benchmark.h>

#ifdef _WIN32
#include <windows.h>
#endif

int main(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    return VTL_opencl_benchmark_run();
}