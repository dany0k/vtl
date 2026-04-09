# FFmpeg.cmake
#
# Подключает FFmpeg из external_libs/ffmpeg (shared libraries).
# Создаёт INTERFACE-таргет 'ffmpeg'.

set(FFMPEG_LIB_DIR "${CMAKE_SOURCE_DIR}/external_libs/ffmpeg/lib")
set(FFMPEG_INC_DIR "${CMAKE_SOURCE_DIR}/external_libs/ffmpeg/include")

add_library(ffmpeg INTERFACE)

target_include_directories(ffmpeg INTERFACE "${FFMPEG_INC_DIR}")

target_link_directories(ffmpeg INTERFACE "${FFMPEG_LIB_DIR}")

target_link_libraries(ffmpeg INTERFACE
    avfilter avformat avcodec swscale swresample avutil
    m pthread
)

# RPATH — чтобы исполняемый файл находил .so из external_libs при запуске
set(CMAKE_INSTALL_RPATH
    "${FFMPEG_LIB_DIR}"
    "${CMAKE_SOURCE_DIR}/external_libs/curl/lib"
    "${CMAKE_SOURCE_DIR}/external_libs/openssl/lib"
    "${CMAKE_SOURCE_DIR}/external_libs/postgresql/lib"
)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

message(STATUS "FFmpeg: using shared libraries from external_libs/ffmpeg")
