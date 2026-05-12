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

### 2. Сборка

#### Toolchain

На любой Linux нужен только **компилятор C11 и CMake**. Никаких apt/brew/vcpkg-установок библиотек проекта.

| Платформа | Команда установки toolchain |
|---|---|
| Ubuntu / Debian / WSL | `sudo apt install build-essential cmake` |
| Fedora / RHEL | `sudo dnf install gcc make cmake` |
| Arch | `sudo pacman -S base-devel cmake` |

#### Вариант A — минимальная сборка (рекомендуется для проверки)

Собирает **только AsciiDoc-парсер** (дипломная фича). Не использует FFmpeg / curl / openssl / postgres вообще — гарантированно работает на любой чистой Linux x86_64.

```bash
cmake -S . -B build -DVTL_MINIMAL_BUILD=ON
cmake --build build
./app/VTL
```

На выходе — demo AsciiDoc парсера и бенчмарк параллелизма.

#### Вариант B — полная сборка

Дополнительно собирает Telegram / Reddit / FFmpeg-аудио / PostgreSQL-историю. Все библиотеки берутся из `external_libs/` через `IMPORTED`-таргеты + `RUNPATH`.

```bash
cmake -S . -B build
cmake --build build
./run.sh   # ставит LD_LIBRARY_PATH для транзитивных FFmpeg-deps
```

⚠️ Полная сборка работает **только на Linux-системе с медиаподдержкой** (Ubuntu Desktop / любая dev-машина с уже установленным мультимедиа-стеком). FFmpeg-shared-libs из `external_libs/` тянут транзитивно `libvpx`, `libdav1d`, `libopus`, `libmp3lame` и ещё ~50 опциональных кодеков, которых на голом server-Ubuntu может не быть.

На совсем голой системе либо используйте `-DVTL_MINIMAL_BUILD=ON`, либо доставьте через `apt install ffmpeg` (это подтянет нужные deps, но без библиотек проекта — только runtime-окружение FFmpeg).

#### Поддерживаемые платформы

| Платформа | Полная сборка | Файлы в репо |
|---|---|---|
| Linux x86_64 (Ubuntu / Debian / Fedora / Arch) | ✅ | `external_libs/{ffmpeg,curl,openssl,postgresql}/lib/*.so` |
| WSL Ubuntu | ✅ | (Linux x86_64 под капотом) |
| Windows x86_64 (MinGW, например winlibs.com) | ✅ | `external_libs/windows/{bin,lib,include}/` |
| macOS arm64 (Apple Silicon, Sonoma+) | ✅ | `external_libs/macos/{lib,include}/` |
| Windows + MSVC | ❌ pthread не поддерживается | — |

Под каждую систему — **та же команда** `cmake -S . -B build && cmake --build build`. CMake сам выбирает нужный набор `IMPORTED`-таргетов по `CMAKE_SYSTEM_NAME`.

```bash
# Linux (Ubuntu пример) — нужен только toolchain
sudo apt install build-essential cmake
cmake -S . -B build && cmake --build build
./app/VTL

# macOS — нужен Xcode Command Line Tools (даёт clang + cmake)
xcode-select --install
cmake -S . -B build && cmake --build build
./app/VTL

# Windows — нужен MinGW-w64 (например с winlibs.com — без MSYS2)
cmake -S . -B build && cmake --build build
.\app\VTL.exe
```

Если нужна **только дипломная фича** (AsciiDoc парсер без FFmpeg/curl/postgres) — добавьте `-DVTL_MINIMAL_BUILD=ON`. Этот режим работает даже без `external_libs/` целиком.

### 3. Сборка из CLion (опционально — для Windows через WSL)

Если работаете в CLion на Windows и не хотите возиться с MinGW — собирайте через WSL-тулчейн.

1. Установите WSL Ubuntu и Linux-зависимости (см. § 2 → Linux / WSL Ubuntu)

2. В CLion: `File -> Open` → папка проекта

3. `Settings -> Build, Execution, Deployment -> Toolchains` → `+` → **WSL** → выберите **Ubuntu** → перетащите вверх

4. `Settings -> Build, Execution, Deployment -> CMake` → **Toolchain** = WSL

5. `Run -> Edit Configurations...` → таргет **VTL**:
   - **Working directory**: `$ProjectFileDir$` (обязательно — иначе не найдёт `text.md`)
   - **Environment variables**: `TG_BOT_TOKEN=<токен>;TG_CHAT_ID=<chat_id>`

6. `File -> Reload CMake Project`

7. **Build** `Ctrl+F9`, **Run** `Shift+F10`

**Если что-то пошло не так:**

- `No such file or directory` при сборке — `File -> Reload CMake Project`
- Segfault (exit 139) — проверьте Working directory = `$ProjectFileDir$`
- Зависает при отправке в Telegram — проверьте доступ из WSL:
  ```bash
  wsl curl -s https://api.telegram.org
  ```
  Нет? Добавьте в WSL `/etc/hosts`:
  ```
  149.154.167.220 api.telegram.org
  ```
  И в `C:\Users\<user>\.wslconfig`:
  ```ini
  [wsl2]
  networkingMode=mirrored
  ```
  Перезапустите: `wsl --shutdown`.

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

Все внешние библиотеки находятся **в папке проекта** (`external_libs/`) и подключаются через CMake как `IMPORTED`-таргеты — `find_package` / `pkg-config` не используются. Единственные требования к системе — компилятор C11 (gcc/clang) и cmake.

Runtime-поиск `.so` идёт через `RUNPATH` бинаря (`$ORIGIN/../external_libs/.../lib`) — не нужно ставить `LD_LIBRARY_PATH` или копировать библиотеки в систему.

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
