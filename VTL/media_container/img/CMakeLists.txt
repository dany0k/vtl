# Создаем библиотеку для обработки изображений
add_library(VTL_img STATIC
    VTL_img_core.c
    VTL_img_filters.c
    VTL_img_utils.c
)

# Заголовочные файлы
target_include_directories(VTL_img
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/external_libs/ffmpeg/include
)

# Линкуем с FFmpeg (таргет из cmake/FFmpeg.cmake)
target_link_libraries(VTL_img PRIVATE ffmpeg)

# Флаги компиляции
target_compile_options(VTL_img PRIVATE
    -Wall -Wextra -Werror -Wno-unused-parameter
)

target_compile_definitions(VTL_img PRIVATE
    _GNU_SOURCE
    VTL_IMG_EXPORTS
)

set_target_properties(VTL_img PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)
