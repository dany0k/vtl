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

### Linux

```bash
sudo apt install build-essential cmake
cmake -S . -B build
cmake --build build
./app/VTL
```

### macOS

```bash
xcode-select --install
brew install cmake
cmake -S . -B build
cmake --build build
./app/VTL
```

### Windows

Сборка нативно через MSVC (Visual Studio). MinGW/MSYS2/WSL не требуются.

```powershell
cmake -S . -B build
cmake --build build
.\app\VTL.exe
```

## Запуск

В корне проекта должны лежать:

- `text.md` — текст публикации
- `audio_ariel.mp3`, `audio_styuardessa.mp3`, `audio_xanadu.mp3` — в репо не закоммичены, положить вручную

Переменные окружения для Telegram:

```bash
export TG_BOT_TOKEN="<токен от @BotFather>"
export TG_CHAT_ID="<id из getUpdates>"
./app/VTL
```

## Структура проекта

```
VTL/
  publication/        — пайплайн публикации (текст, аудио)
  content_platform/   — Telegram, Reddit
  media_container/    — аудио, видео, субтитры, изображения
  user/               — история публикаций
  utils/              — файлы, строки, шифрование, HTTP, БД
external_sources/parson/                          — JSON-парсер
external_libs/{ffmpeg,curl,openssl,postgresql}/   — Linux .so
external_libs/windows/                            — Windows .dll + .lib (MSVC) / .dll.a (MinGW)
external_libs/macos/                              — macOS .dylib
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
