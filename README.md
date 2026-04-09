# VTL — Video/Text/Audio publication Library

Библиотека на C11 для автоматической публикации медиаконтента (текст, аудио, видео, изображения) на различные контент-платформы: Telegram, Reddit, Web и другие.

## Возможности

- **Текстовый пайплайн** — генерация текстовых файлов в форматах Telegram MarkdownV2, HTML, BBCode; отправка в Telegram
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

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

Исполняемый файл появится в `app/VTL`.

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

Перед запуском создайте файл `text.md` в корне проекта с текстом публикации:

```
Привет из VTL!
```

Задайте переменные окружения для Telegram:

```bash
export TG_BOT_TOKEN="<токен бота>"
export TG_CHAT_ID="<id чата>"
./app/VTL
```

Программа:
1. Читает `text.md`, генерирует форматированные файлы (`.t_md`, `.html`)
2. Отправляет текст как сообщение в Telegram
3. Отправляет аудиофайл в Telegram с подписью из `text.md`

Для получения `TG_CHAT_ID` — напишите боту в Telegram, затем:
```bash
curl https://api.telegram.org/bot<TOKEN>/getUpdates
```
В ответе найдите `"chat":{"id": ...}`.

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

Команда проекта VTL, ТКСВ.
