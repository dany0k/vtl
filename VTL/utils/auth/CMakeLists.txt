# VTL_utils_auth — тонкий wrapper над ed25519 (sign/verify, генерация ключевых пар,
# save/load в файл). Используется компонентами content_platform для подписи
# challenge/response при общении с внешними API.

set(VTL_UTILS_AUTH_SOURCES
    VTL_utils_auth.c
    VTL_utils_auth_keys.c
)

add_library(VTL_utils_auth STATIC ${VTL_UTILS_AUTH_SOURCES})

target_include_directories(VTL_utils_auth PUBLIC
    "${CMAKE_SOURCE_DIR}"
)

target_link_libraries(VTL_utils_auth PRIVATE vtl_ed25519)
