# MinGW-w64 cross-compile toolchain для сборки Windows-бинаря из Linux.
# Используется так:
#   cmake -S . -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-toolchain.cmake
#   cmake --build build-win
#
# На самой Windows-машине этот файл НЕ нужен — там CMake сам найдёт MinGW.

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER   x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER  x86_64-w64-mingw32-windres)

# Поиск библиотек/headers — только в путях проекта и mingw sysroot,
# не в системе хоста.
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# POSIX-flavour mingw — даёт pthread. Без этого pthread не найдётся.
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
