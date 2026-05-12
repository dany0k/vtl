#!/bin/bash
# Сборка и запуск VTL — всё одной командой.
#
# Все зависимости (FFmpeg, libcurl, OpenSSL, libpq) лежат в external_libs/.
# Никакие apt/brew/vcpkg ставить НЕ нужно — только build-essential + cmake.
#
# LD_LIBRARY_PATH ставим вручную потому что RUNPATH бинаря работает только
# для прямых зависимостей. Транзитивные (FFmpeg тянет libswresample и т.п.)
# ищутся в RUNPATH самих .so — а у них он пустой. LD_LIBRARY_PATH применяется
# ко всему дереву загрузки и спасает от этой проблемы.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

cmake -S "$SCRIPT_DIR" -B "$SCRIPT_DIR/build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$SCRIPT_DIR/build" -j

export LD_LIBRARY_PATH="\
$SCRIPT_DIR/external_libs/ffmpeg/lib:\
$SCRIPT_DIR/external_libs/curl/lib:\
$SCRIPT_DIR/external_libs/openssl/lib:\
$SCRIPT_DIR/external_libs/postgresql/lib:\
${LD_LIBRARY_PATH:-}"

exec "$SCRIPT_DIR/app/VTL" "$@"
