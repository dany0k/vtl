# VTL_media_container_encoding — видеокодирующая библиотека (H.264/H.265/AV1).
# Базовый CPU-encoder через FFmpeg, опциональный Vulkan-encoder.

option(VTL_MEDIA_CONTAINER_ENCODING_WITH_VK "Включить Vulkan-encoder (требует Vulkan SDK + FFmpeg с --enable-vulkan)" OFF)
option(VTL_MEDIA_CONTAINER_ENCODING_BUILD_DEMO "Собирать examples/demo_all_modes" OFF)

set(VTL_MEDIA_CONTAINER_ENCODING_SOURCES
    VTL_media_container_encoding_core.c
    VTL_media_container_encoding_cpu.c
    VTL_media_container_encoding_loader.c
    VTL_media_container_encoding_transcoder.c
)

if(VTL_MEDIA_CONTAINER_ENCODING_WITH_VK)
    find_package(Vulkan QUIET)
    if(NOT Vulkan_FOUND)
        message(FATAL_ERROR
            "VTL_MEDIA_CONTAINER_ENCODING_WITH_VK=ON, но Vulkan SDK не найден. "
            "Поставьте Vulkan SDK либо отключите опцию.")
    endif()
    list(APPEND VTL_MEDIA_CONTAINER_ENCODING_SOURCES VTL_media_container_encoding_vk.c)
endif()

add_library(VTL_media_container_encoding STATIC ${VTL_MEDIA_CONTAINER_ENCODING_SOURCES})

target_include_directories(VTL_media_container_encoding PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/VTL"
)

target_link_libraries(VTL_media_container_encoding PRIVATE ffmpeg)

if(VTL_MEDIA_CONTAINER_ENCODING_WITH_VK)
    target_link_libraries(VTL_media_container_encoding PRIVATE Vulkan::Vulkan)
endif()

if(VTL_MEDIA_CONTAINER_ENCODING_BUILD_DEMO)
    add_executable(VTL_media_container_encoding_demo examples/VTL_media_container_encoding_demo_all_modes.c)
    target_link_libraries(VTL_media_container_encoding_demo PRIVATE VTL_media_container_encoding)
endif()
