# VTL_auth — тонкий wrapper над ed25519 (sign/verify, генерация ключевых пар,
# save/load в файл). Используется компонентами content_platform для подписи
# challenge/response при общении с внешними API.

set(VTL_AUTH_SOURCES
    VTL_auth.c
    VTL_keys.c
)

add_library(VTL_auth STATIC ${VTL_AUTH_SOURCES})

target_include_directories(VTL_auth PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(VTL_auth PRIVATE vtl_ed25519)
