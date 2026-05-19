# Discord Markdown ↔ внутренний формат проекта.
# Структура идентична VTL_asciidoc — самодостаточная статическая библиотека,
# без внешних зависимостей кроме pthreads на UNIX.

add_library(VTL_discord STATIC
    VTL_publication_text_op_discord.c
    VTL_publication_text_op_discord_data.c
    VTL_publication_text_op_discord_scan.c
    VTL_publication_text_op_discord_convert.c
    # Структуры VTL_publication_Text / MarkedText
    ../VTL_publication_text_data.c
)

target_include_directories(VTL_discord PUBLIC ${CMAKE_SOURCE_DIR})

# pthread нужен только на UNIX; под Windows compat.h уходит в WinAPI
if(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(VTL_discord PUBLIC Threads::Threads)
endif()

set_target_properties(VTL_discord PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

if(MSVC)
    target_compile_options(VTL_discord PRIVATE /W4 /utf-8)
else()
    target_compile_options(VTL_discord PRIVATE
        -Wall -Wextra
        -Wno-error=implicit-function-declaration
        -Wno-error=incompatible-pointer-types
        -Wno-error=int-conversion
        -Wno-unused-parameter
    )
endif()
