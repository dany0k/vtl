# VTL_vencode — видеокодирующая библиотека (H.264/H.265/AV1).
# Базовый CPU-encoder через FFmpeg, опциональный Vulkan-encoder.

option(VTL_VENCODE_WITH_VK "Включить Vulkan-encoder (требует Vulkan SDK + FFmpeg с --enable-vulkan)" OFF)
option(VTL_VENCODE_BUILD_DEMO "Собирать examples/demo_all_modes" OFF)

set(VTL_VENCODE_SOURCES
    VTL_vencode_core.c
    VTL_vencode_cpu.c
    VTL_vencode_loader.c
    VTL_vencode_transcoder.c
)

if(VTL_VENCODE_WITH_VK)
    find_package(Vulkan QUIET)
    if(NOT Vulkan_FOUND)
        message(FATAL_ERROR
            "VTL_VENCODE_WITH_VK=ON, но Vulkan SDK не найден. "
            "Поставьте Vulkan SDK либо отключите опцию.")
    endif()
    list(APPEND VTL_VENCODE_SOURCES VTL_vencode_vk.c)
endif()

add_library(VTL_vencode STATIC ${VTL_VENCODE_SOURCES})

target_include_directories(VTL_vencode PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/VTL"
)

target_link_libraries(VTL_vencode PRIVATE ffmpeg)

if(VTL_VENCODE_WITH_VK)
    target_link_libraries(VTL_vencode PRIVATE Vulkan::Vulkan)
endif()

if(VTL_VENCODE_BUILD_DEMO)
    add_executable(vencode_demo examples/demo_all_modes.c)
    target_link_libraries(vencode_demo PRIVATE VTL_vencode)
endif()
