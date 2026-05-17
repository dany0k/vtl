find_package(OpenCL REQUIRED)

add_library(VTL_sub_opencl_ctx              OBJECT VTL_sub_opencl_ctx.c)
add_library(VTL_sub_opencl_clean_tags       OBJECT VTL_sub_opencl_clean_tags.c)
add_library(VTL_sub_opencl_strip_tags       OBJECT VTL_sub_opencl_strip_tags.c)
add_library(VTL_sub_opencl_hash_strings     OBJECT VTL_sub_opencl_hash_strings.c)
add_library(VTL_sub_opencl_string_stats     OBJECT VTL_sub_opencl_string_stats.c)
add_library(VTL_sub_opencl_format_time      OBJECT VTL_sub_opencl_format_time.c)
add_library(VTL_sub_opencl_apply_ass_style  OBJECT VTL_sub_opencl_apply_ass_style.c)
add_library(VTL_sub_opencl_squeeze_repeats  OBJECT VTL_sub_opencl_squeeze_repeats.c)
add_library(VTL_sub_opencl_format_numbers   OBJECT VTL_sub_opencl_format_numbers.c)
add_library(VTL_sub_opencl_split_long_lines OBJECT VTL_sub_opencl_split_long_lines.c)
add_library(VTL_sub_opencl_detect_encoding  OBJECT VTL_sub_opencl_detect_encoding.c)

# Все OBJECT-таргеты opencl нуждаются в CMAKE_SOURCE_DIR для <VTL/VTL_app_result.h>
foreach(_tgt
    VTL_sub_opencl_ctx
    VTL_sub_opencl_strip_tags
    VTL_sub_opencl_clean_tags
    VTL_sub_opencl_hash_strings
    VTL_sub_opencl_string_stats
    VTL_sub_opencl_format_time
    VTL_sub_opencl_apply_ass_style
    VTL_sub_opencl_squeeze_repeats
    VTL_sub_opencl_format_numbers
    VTL_sub_opencl_split_long_lines
    VTL_sub_opencl_detect_encoding
)
    target_include_directories(${_tgt} PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/VTL)
    target_compile_definitions(${_tgt} PRIVATE CL_TARGET_OPENCL_VERSION=120)
    target_link_libraries(${_tgt} PRIVATE OpenCL::OpenCL)
endforeach()

# Бенчмарк — отдельный исполняемый файл
# Сборка: cmake --build . --target VTL_opencl_benchmark
# Запуск: ./app/VTL_opencl_benchmark
add_executable(VTL_opencl_benchmark
    VTL_sub_opencl_benchmark_main.c
    VTL_sub_opencl_benchmark.c
    VTL_sub_opencl_ctx.c
    VTL_sub_opencl_strip_tags.c
)
target_include_directories(VTL_opencl_benchmark PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/VTL)
target_compile_definitions(VTL_opencl_benchmark PRIVATE CL_TARGET_OPENCL_VERSION=120)
target_link_libraries(VTL_opencl_benchmark PRIVATE OpenCL::OpenCL)
