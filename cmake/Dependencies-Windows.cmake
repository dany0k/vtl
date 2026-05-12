# Windows-сборка через MinGW-w64. Все .dll/.dll.a/.lib/.h лежат в external_libs/windows/.
# Не зависит от MSYS2, vcpkg, system-installed libraries — всё в репо.
#
# Что лежит:
#   external_libs/windows/bin/      — runtime DLL (копируются в app/ после билда)
#   external_libs/windows/lib/      — import libraries (.dll.a, .lib)
#   external_libs/windows/include/  — headers

set(WIN_DIR "${EXTERNAL_LIBS_DIR}/windows")
set(WIN_BIN "${WIN_DIR}/bin")
set(WIN_LIB "${WIN_DIR}/lib")
set(WIN_INC "${WIN_DIR}/include")

# ============================================================
# FFmpeg (DLL от BtbN/FFmpeg-Builds, формат avXXX-MAJOR.dll)
# ============================================================
add_library(ffmpeg INTERFACE)
target_include_directories(ffmpeg INTERFACE "${WIN_INC}")

# Имя DLL содержит major-версию: avcodec-62.dll, avformat-62.dll и т.д.
# .lib файлы — без версии: avcodec.lib и т.д. (COFF, совместим с MinGW)
macro(_vtl_win_ffmpeg comp ver)
    add_library(ffmpeg_${comp} SHARED IMPORTED)
    set_target_properties(ffmpeg_${comp} PROPERTIES
        IMPORTED_LOCATION "${WIN_BIN}/${comp}-${ver}.dll"
        IMPORTED_IMPLIB   "${WIN_LIB}/${comp}.lib"
    )
    target_link_libraries(ffmpeg INTERFACE ffmpeg_${comp})
endmacro()

_vtl_win_ffmpeg(avcodec    62)
_vtl_win_ffmpeg(avformat   62)
_vtl_win_ffmpeg(avutil     60)
_vtl_win_ffmpeg(avfilter   11)
_vtl_win_ffmpeg(swscale     9)
_vtl_win_ffmpeg(swresample  6)

# ============================================================
# libcurl
# ============================================================
add_library(CURL::libcurl SHARED IMPORTED)
set_target_properties(CURL::libcurl PROPERTIES
    IMPORTED_LOCATION "${WIN_BIN}/libcurl-4.dll"
    IMPORTED_IMPLIB   "${WIN_LIB}/libcurl.dll.a"
    INTERFACE_INCLUDE_DIRECTORIES "${WIN_INC}"
)

# ============================================================
# OpenSSL
# ============================================================
add_library(OpenSSL::Crypto SHARED IMPORTED)
set_target_properties(OpenSSL::Crypto PROPERTIES
    IMPORTED_LOCATION "${WIN_BIN}/libcrypto-3-x64.dll"
    IMPORTED_IMPLIB   "${WIN_LIB}/libcrypto.dll.a"
    INTERFACE_INCLUDE_DIRECTORIES "${WIN_INC}"
)

add_library(OpenSSL::SSL SHARED IMPORTED)
set_target_properties(OpenSSL::SSL PROPERTIES
    IMPORTED_LOCATION "${WIN_BIN}/libssl-3-x64.dll"
    IMPORTED_IMPLIB   "${WIN_LIB}/libssl.dll.a"
    INTERFACE_INCLUDE_DIRECTORIES "${WIN_INC}"
    INTERFACE_LINK_LIBRARIES OpenSSL::Crypto
)

# ============================================================
# libpq (PostgreSQL)
# ============================================================
add_library(PostgreSQL::PostgreSQL SHARED IMPORTED)
set_target_properties(PostgreSQL::PostgreSQL PROPERTIES
    IMPORTED_LOCATION "${WIN_BIN}/libpq.dll"
    IMPORTED_IMPLIB   "${WIN_LIB}/libpq.dll.a"
    INTERFACE_INCLUDE_DIRECTORIES "${WIN_INC}"
)

# ============================================================
# winpthread — pthread для MinGW. Содержит clock_gettime64.
# Используем НАШ libwinpthread из external_libs/, а не системный
# (системный mingw на Ubuntu может быть win32-flavour без pthread).
# ============================================================
add_library(VTL::winpthread SHARED IMPORTED)
set_target_properties(VTL::winpthread PROPERTIES
    IMPORTED_LOCATION "${WIN_BIN}/libwinpthread-1.dll"
    IMPORTED_IMPLIB   "${WIN_LIB}/libwinpthread.dll.a"
)

# ============================================================
# Копирование всех DLL в app/ рядом с .exe — Windows ищет DLL там.
# Делается на этапе configure: при первой настройке + при изменении файлов в bin/.
# ============================================================
file(MAKE_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
file(GLOB _VTL_WIN_DLLS "${WIN_BIN}/*.dll")
file(COPY ${_VTL_WIN_DLLS} DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

message(STATUS "VTL Windows: ${EXTERNAL_LIBS_DIR}/windows (FFmpeg ${_VTL_FFMPEG_AVCODEC}, curl, openssl, libpq)")
