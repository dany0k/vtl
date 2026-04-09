# VTL — Video/Text/Audio publication Library

Библиотека на C11 для автоматической публикации медиаконтента (текст, аудио, видео, изображения) на различные контент-платформы: Telegram, Reddit, Web и другие.

## Возможности

- **Текстовый пайплайн** — генерация текстовых файлов в форматах Telegram MarkdownV2, HTML, BBCode; отправка в Telegram
- **Аудио пайплайн** — чтение, перекодирование (FFmpeg) и отправка аудиофайлов с подписью в Telegram
- **Видео** — структура для видеоконтейнеров и субтитров (SRT парсинг, наложение, конвертация, стилизация)
- **Изображения** — обработка через FFmpeg (фильтры, утилиты)
- **Reddit** — модуль публикации через Reddit API
- **БД** — сохранение истории публикаций в PostgreSQL

## Структура проекта

```
VTL/
  publication/           — пайплайн публикации (текст, аудио)
  content_platform/
    tg/                  — Telegram Bot API (sendMessage, sendAudio, sendMediaGroup, ...)
    reddit/              — Reddit API
    infra/               — конфигурации платформ (аудио-параметры, текстовые конфиги)
  media_container/
    audio/               — аудио: чтение, запись, перекодирование
    video/               — видео: данные
    img/                 — изображения: ядро, фильтры, утилиты
    sub/                 — субтитры: SRT парсинг, наложение, конвертация, стили
  user/                  — пользователь и история публикаций
  utils/                 — файлы, строки, шифрование, логирование, HTTP-клиент, БД
external_sources/
  parson/                — JSON-библиотека (единственная, собирается из исходников)
external_libs/
  ffmpeg/                — FFmpeg (shared libraries + headers)
  curl/                  — libcurl (shared library + headers)
  openssl/               — OpenSSL (shared libraries + headers)
  postgresql/            — libpq (shared library + headers)
```

## Сборка

### Требования

- CMake >= 3.25
- GCC с поддержкой C11
- Linux / WSL (Ubuntu)

### Команды

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

Исполняемый файл: `app/VTL`

### Сборка и запуск из CLion

1. Settings -> Build -> Toolchains -> добавить WSL (Ubuntu)
2. Settings -> Build -> CMake -> Toolchain: WSL
3. Run/Debug Configuration -> Working directory: `$ProjectFileDir$`
4. Environment variables: `TG_BOT_TOKEN=...;TG_CHAT_ID=...`

## Запуск

```bash
export TG_BOT_TOKEN="<токен бота>"
export TG_CHAT_ID="<id чата>"
./app/VTL
```

Программа:
1. Читает `text.md` и генерирует форматированные файлы (`.t_md`, `.html`)
2. Отправляет текст в Telegram
3. Отправляет аудиофайл в Telegram с подписью

## Тестирование обработки изображений

```bash
mkdir -p test_images
cp doc/Lenna.png test_images/
cp doc/main.c .
cmake --build build
./app/VTL
```

Результат: `test_images/Lenna_processed.png`

## Внешние библиотеки

Все внешние библиотеки находятся в папке проекта и подключаются через CMake:

| Библиотека | Расположение | Назначение |
|---|---|---|
| parson | `external_sources/parson/` | JSON (сборка из исходников) |
| FFmpeg | `external_libs/ffmpeg/` | Аудио/видео/изображения |
| libcurl | `external_libs/curl/` | HTTP-запросы (Telegram API, Reddit API) |
| OpenSSL | `external_libs/openssl/` | TLS для HTTP |
| libpq | `external_libs/postgresql/` | PostgreSQL |

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

Команда проекта VTL, ТКСВ.
