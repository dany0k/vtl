# Windows-сборка под MSVC. Все .dll/.lib/.h лежат в external_libs/windows/.
# Не зависит от MSYS2, vcpkg, MinGW, system-installed libraries — всё в репо.
#
# Что лежит:
#   external_libs/windows/bin/      — runtime DLL (копируются в app/ после билда)
#   external_libs/windows/lib/      — import libraries (.lib, COFF — MSVC)
#   external_libs/windows/include/  — headers
#
# Источники бинарей (все MSVC-собранные, без MinGW):
#   FFmpeg 7.1   — github.com/ShiftMediaProject/FFmpeg (libffmpeg_7.1_msvc17_x64.zip)
#   OpenSSL 3.6  — download.firedaemon.com (FireDaemon OpenSSL)
#   libcurl 8.19 — EnterpriseDB PostgreSQL bundle (bin/libcurl.dll)
#   libpq        — собран локально из postgresql-18.3 source + meson (-Dnls=disabled)

if(WIN32 AND NOT MSVC)
    message(FATAL_ERROR
        "Под Windows проект собирается только MSVC (Visual Studio). "
        "MinGW/MSYS2/Cygwin не поддерживаются.")
endif()

set(WIN_DIR "${EXTERNAL_LIBS_DIR}/windows")
set(WIN_BIN "${WIN_DIR}/bin")
set(WIN_LIB "${WIN_DIR}/lib")
set(WIN_INC "${WIN_DIR}/include")

# ============================================================
# FFmpeg (ShiftMediaProject MSVC build — DLL без version-суффикса)
# ============================================================
add_library(ffmpeg INTERFACE)
target_include_directories(ffmpeg INTERFACE "${WIN_INC}")

macro(_vtl_win_ffmpeg comp)
    add_library(ffmpeg_${comp} SHARED IMPORTED)
    set_target_properties(ffmpeg_${comp} PROPERTIES
        IMPORTED_LOCATION "${WIN_BIN}/${comp}.dll"
        IMPORTED_IMPLIB   "${WIN_LIB}/${comp}.lib"
    )
    target_link_libraries(ffmpeg INTERFACE ffmpeg_${comp})
endmacro()

_vtl_win_ffmpeg(avcodec)
_vtl_win_ffmpeg(avformat)
_vtl_win_ffmpeg(avutil)
_vtl_win_ffmpeg(avfilter)
_vtl_win_ffmpeg(swscale)
_vtl_win_ffmpeg(swresample)

# ============================================================
# libcurl
# ============================================================
add_library(CURL::libcurl SHARED IMPORTED)
set_target_properties(CURL::libcurl PROPERTIES
    IMPORTED_LOCATION "${WIN_BIN}/libcurl.dll"
    IMPORTED_IMPLIB   "${WIN_LIB}/libcurl.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${WIN_INC}"
)

# ============================================================
# OpenSSL
# ============================================================
add_library(OpenSSL::Crypto SHARED IMPORTED)
set_target_properties(OpenSSL::Crypto PROPERTIES
    IMPORTED_LOCATION "${WIN_BIN}/libcrypto-3-x64.dll"
    IMPORTED_IMPLIB   "${WIN_LIB}/libcrypto.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${WIN_INC}"
)

add_library(OpenSSL::SSL SHARED IMPORTED)
set_target_properties(OpenSSL::SSL PROPERTIES
    IMPORTED_LOCATION "${WIN_BIN}/libssl-3-x64.dll"
    IMPORTED_IMPLIB   "${WIN_LIB}/libssl.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${WIN_INC}"
    INTERFACE_LINK_LIBRARIES OpenSSL::Crypto
)

# ============================================================
# libpq (PostgreSQL)
# ============================================================
add_library(PostgreSQL::PostgreSQL SHARED IMPORTED)
set_target_properties(PostgreSQL::PostgreSQL PROPERTIES
    IMPORTED_LOCATION "${WIN_BIN}/libpq.dll"
    IMPORTED_IMPLIB   "${WIN_LIB}/libpq.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${WIN_INC}"
)

# ============================================================
# Копирование всех DLL в app/ рядом с .exe — Windows ищет DLL там.
# Делается на этапе configure: при первой настройке + при изменении файлов в bin/.
# ============================================================
file(MAKE_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
file(GLOB _VTL_WIN_DLLS "${WIN_BIN}/*.dll")
file(COPY ${_VTL_WIN_DLLS} DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

message(STATUS "VTL Windows: ${EXTERNAL_LIBS_DIR}/windows (FFmpeg 7.1, curl 8.19, openssl 3.6, libpq 18.3)")
