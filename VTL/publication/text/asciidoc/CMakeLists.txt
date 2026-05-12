# Самодостаточная под-библиотека модуля publication.
# Не зависит ни от utils, ни от curl/openssl/postgres/ffmpeg.
# Используется при VTL_MINIMAL_BUILD=ON для сборки на чистой ОС.
# Работает на Linux, macOS, Windows (MinGW). MSVC не поддерживается — pthread'а нет.

add_library(VTL_asciidoc STATIC
    VTL_publication_text_op_asciidoc.c
    VTL_publication_text_op_asciidoc_data.c
    VTL_publication_text_op_asciidoc_scan.c
    VTL_publication_text_op_asciidoc_convert.c
    infra/VTL_publication_text_op_asciidoc_load.c
    # Парсер использует типы VTL_publication_Text / MarkedText
    # из publication/text/VTL_publication_text_data.h — поэтому подключаем
    # data.c (структуры без внешних зависимостей).
    ../VTL_publication_text_data.c
)

target_include_directories(VTL_asciidoc PUBLIC ${CMAKE_SOURCE_DIR})

# Threads::Threads — кросс-платформенный pthread:
#   - Linux/macOS → -lpthread
#   - Windows MinGW → -lpthread (winpthreads)
#   - Windows MSVC → fail (тут pthread'а нет, нужна другая реализация)
find_package(Threads REQUIRED)
target_link_libraries(VTL_asciidoc PUBLIC Threads::Threads)

set_target_properties(VTL_asciidoc PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

if(MSVC)
    target_compile_options(VTL_asciidoc PRIVATE /W4 /utf-8)
else()
    target_compile_options(VTL_asciidoc PRIVATE
        -Wall -Wextra
        -Wno-error=implicit-function-declaration
        -Wno-error=incompatible-pointer-types
        -Wno-error=int-conversion
        -Wno-unused-parameter
    )
endif()
