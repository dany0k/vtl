# VTL — Video/Text/Audio publication Library

Библиотека на C11 для автоматической публикации медиаконтента (текст, аудио, видео, изображения) на различные контент-платформы: Telegram, Reddit, Web и другие.

## Возможности

- **Текстовый пайплайн** — генерация текстовых файлов в форматах Telegram MarkdownV2, HTML, BBCode; отправка в Telegram
- **AsciiDoc парсер** — параллельный (pthread) разбор 15 типов разметки в `MarkedText` с двумя уровнями параллелизма (по сканерам и по файлам)
- **Аудио пайплайн** — чтение, перекодирование (FFmpeg) и отправка аудиофайлов с подписью в Telegram
- **Видео** — структура для видеоконтейнеров и субтитров (SRT парсинг, наложение, конвертация, стилизация)
- **Изображения** — обработка через FFmpeg (фильтры, утилиты)
- **Reddit** — модуль публикации через Reddit API
- **БД** — сохранение истории публикаций в PostgreSQL

## Быстрый старт

### 1. Клонирование

```bash
git clone <url-репозитория>
cd vtl
```

### 2. Вариант A: Сборка из терминала (Linux / WSL Ubuntu)

На чистой Ubuntu сначала поставьте toolchain:

```bash
sudo apt update
sudo apt install build-essential cmake pkg-config
```

Затем собираем:

```bash
mkdir build && cd build
cmake ..
cmake --build .
cd ..
```

Исполняемый файл появится в `app/VTL` (в **корне проекта**, не в `build/app/` — поэтому перед запуском `cd ..`).

### 3. Вариант B: Сборка из CLion (Windows + WSL)

#### Первоначальная настройка (один раз)

1. Установите WSL Ubuntu, если не установлен:
   ```powershell
   wsl --install -d Ubuntu
   ```

2. В CLion откройте проект (`File -> Open` -> папка `vtl`)

3. Настройте WSL-тулчейн:
   - `Settings -> Build, Execution, Deployment -> Toolchains`
   - Нажмите `+`, выберите **WSL**
   - Выберите дистрибутив **Ubuntu**
   - Дождитесь пока CLion обнаружит CMake, gcc, g++
   - Перетащите WSL-тулчейн **вверх списка** (чтобы стал дефолтным)

4. Убедитесь что CMake использует WSL:
   - `Settings -> Build, Execution, Deployment -> CMake`
   - Поле **Toolchain** должно быть **WSL** (или WSL Ubuntu)

5. Настройте конфигурацию запуска:
   - `Run -> Edit Configurations...`
   - Выберите таргет **VTL**
   - **Working directory**: `$ProjectFileDir$` (обязательно! иначе не найдёт файлы)
   - **Environment variables**: `TG_BOT_TOKEN=<токен>;TG_CHAT_ID=<chat_id>`

6. Нажмите **File -> Reload CMake Project** (или кнопку Reload в панели CMake внизу)

#### Сборка и запуск

- **Build**: `Ctrl+F9` или кнопка Build (молоток)
- **Run**: `Shift+F10` или кнопка Run (зелёный треугольник)

#### Если что-то пошло не так

- Ошибка `No such file or directory` при сборке — **File -> Reload CMake Project**
- Segfault (exit code 139) — проверьте что Working directory = `$ProjectFileDir$`
- Программа зависает — проверьте что в WSL доступен `api.telegram.org`:
  ```bash
  wsl curl -s https://api.telegram.org
  ```
  Если нет — добавьте в WSL файл `/etc/hosts`:
  ```
  149.154.167.220 api.telegram.org
  ```
  И в `C:\Users\<user>\.wslconfig`:
  ```ini
  [wsl2]
  networkingMode=mirrored
  ```
  После чего перезапустите WSL: `wsl --shutdown` и откройте заново.

### 4. Запуск

#### Что должно лежать в корне проекта

- **`text.md`** — текст публикации. Пример:
  ```
  Привет из VTL!
  ```
- **Три аудиофайла** (имена жёстко прописаны в `main.c`):
  - `audio_ariel.mp3`
  - `audio_styuardessa.mp3`
  - `audio_xanadu.mp3`

  Можно положить любые `mp3` с такими именами. Без них Audio Pipeline вернёт ошибку.
  В репозиторий они **не закоммичены** (`*.mp3` исключены) — нужно положить вручную.

#### Переменные окружения

```bash
export TG_BOT_TOKEN="<токен бота>"
export TG_CHAT_ID="<id чата>"
./app/VTL
```

**Получение `TG_BOT_TOKEN`:**

1. В Telegram напишите `@BotFather`
2. Команда `/newbot`, задайте имя и username бота
3. BotFather пришлёт токен вида `123456789:ABCdefGhIJklmNOpqrSTUvwxyz`

**Получение `TG_CHAT_ID`:**

Напишите своему боту любое сообщение, затем:
```bash
curl https://api.telegram.org/bot<TOKEN>/getUpdates
```
В ответе найдите `"chat":{"id": ...}`.

#### Что делает программа

1. **AsciiDoc демо** — парсит in-memory пример и печатает разбор по частям с флагами BOLD/ITALIC/STRIKE
2. **Бенчмарк параллелизма** — Sequential vs Parallel для 15 сканеров (512 KB документ) и для batch'а (8 файлов × 128 KB), считает Speedup и Efficiency
3. **Text Pipeline** — читает `text.md`, генерирует `.t_md` / `.html`, отправляет в Telegram
4. **Audio Pipeline** — отправляет один из аудиофайлов в Telegram с подписью из `text.md`

## Структура проекта

```
VTL/
  publication/           — пайплайн публикации (текст, аудио)
    text/
      asciidoc/          — параллельный парсер AsciiDoc
      bbcode/            — парсер BBCode
      infra/             — чтение/запись текстовых файлов
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

## Внешние библиотеки

Все внешние библиотеки находятся в папке проекта и подключаются через CMake. Никаких системных зависимостей кроме стандартной libc и toolchain.

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

Команда проекта VTL.
