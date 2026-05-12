# VTL — Video/Text/Audio publication Library

Библиотека на C11 для автоматической публикации медиаконтента (текст, аудио, видео, изображения) в Telegram, Reddit и другие платформы.

## Возможности

- Текст: Telegram MarkdownV2, HTML, BBCode → отправка в Telegram
- Аудио: чтение, перекодирование через FFmpeg, отправка с подписью
- Видео и субтитры: SRT парсинг, наложение, конвертация
- Изображения: фильтры и утилиты через FFmpeg
- Reddit API, история публикаций в PostgreSQL

## Сборка

Все библиотеки (FFmpeg, libcurl, OpenSSL, libpq) лежат прямо в `external_libs/`. **Нужен только компилятор и CMake** — никаких `apt install ffmpeg` / `brew install ffmpeg` / `vcpkg`.

### Linux (Ubuntu / Debian / WSL)

```bash
sudo apt install build-essential cmake
git clone <url> && cd vtl
cmake -S . -B build
cmake --build build
./app/VTL
```

Для Fedora — `sudo dnf install gcc make cmake`. Для Arch — `sudo pacman -S base-devel cmake`.

### macOS (Apple Silicon, Sonoma+)

```bash
xcode-select --install
brew install cmake
git clone <url> && cd vtl
cmake -S . -B build
cmake --build build
./app/VTL
```

### Windows (нативно, без WSL/MSYS2)

#### Способ A — через Visual Studio (рекомендуется)

Если уже установлена Visual Studio (Community/Professional) или Build Tools — больше ничего не нужно. CMake сам найдёт MSVC.

Если нет — поставь **Visual Studio Community** с https://visualstudio.microsoft.com/downloads/ (бесплатная), при установке выбери workload **"Desktop development with C++"**.

Также нужен **CMake** — скачай с https://cmake.org/download/ → `Windows x64 Installer`, при установке отметь ✅ **"Add CMake to the system PATH"**.

После — обычная команда:

```powershell
git clone <url>
cd vtl
cmake -S . -B build
cmake --build build
.\app\Debug\VTL.exe
```

⚠️ `.exe` появится в `app\Debug\VTL.exe` (или `app\Release\` если `cmake --build build --config Release`) — особенность VS-генератора.

#### Способ B — через MinGW-w64 (альтернатива, если MSVC не подходит)

Скачай с https://github.com/brechtsanders/winlibs_mingw/releases/latest архив `winlibs-x86_64-posix-seh-gcc-*-mingw-w64ucrt-*-rN.zip`. Распакуй в `C:\` → получится `C:\mingw64\`. Добавь `C:\mingw64\bin` в `Path` (Пуск → "Изменение переменных среды"). Перезапусти PowerShell.

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
.\app\VTL.exe
```

## Запуск

В корне проекта должны лежать:

- `text.md` — текст публикации (любой, например `Привет из VTL!`)
- Три mp3-файла (имена жёстко прописаны в `main.c`): `audio_ariel.mp3`, `audio_styuardessa.mp3`, `audio_xanadu.mp3`. В репо их нет (`*.mp3` исключены), положить вручную.

Переменные окружения для Telegram:

```bash
export TG_BOT_TOKEN="<токен от @BotFather>"
export TG_CHAT_ID="<id из getUpdates>"
./app/VTL
```

Получить `TG_BOT_TOKEN`: написать `@BotFather` → `/newbot`.
Получить `TG_CHAT_ID`: написать своему боту любое сообщение и открыть `https://api.telegram.org/bot<TOKEN>/getUpdates` — там `"chat":{"id":...}`.

## Структура проекта

```
VTL/
  publication/        — пайплайн публикации (текст, аудио)
    text/{asciidoc,bbcode,infra}/
  content_platform/   — Telegram, Reddit, конфиги платформ
  media_container/    — аудио, видео, субтитры, изображения
  user/               — история публикаций
  utils/              — файлы, строки, шифрование, HTTP, БД
external_sources/parson/    — JSON-парсер (собирается из исходников)
external_libs/{ffmpeg,curl,openssl,postgresql}/    — Linux .so
external_libs/windows/      — Windows .dll
external_libs/macos/        — macOS .dylib
```

## Интегрированные модули

| Ветка | Автор | Модуль |
|---|---|---|
| yarovitchuk | Яровитчук | Аудио (чтение, запись, перекодирование) |
| bulachev | Булачев | Enum-ы, исправления опечаток |
| lachugin | Лачугин | BBCode формат текста |
| ivannikov | Иванников | Telegram Bot API |
| ishatalov | Ишаталов | БД (PostgreSQL) |
| fedorov-subtitles-macOS | Федоров | Субтитры (SRT парсинг, наложение, конвертация, стили) |
| borisenkov-reddit | Борисенков | Reddit API, HTTP-клиент |

## Авторы

Команда проекта VTL.
