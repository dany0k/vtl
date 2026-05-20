# Ed25519 — vendored референсная реализация из orlp/ed25519
# (https://github.com/orlp/ed25519). Статическая lib, header-only публичный API.
# Используется через wrapper VTL/utils/auth/.

set(VTL_ED25519_SOURCES
    add_scalar.c
    fe.c
    ge.c
    key_exchange.c
    keypair.c
    sc.c
    seed.c
    sha512.c
    sign.c
    verify.c
)

add_library(vtl_ed25519 STATIC ${VTL_ED25519_SOURCES})

target_include_directories(vtl_ed25519 PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

# MSVC ругается C4146 на "unary minus operator applied to unsigned type" в
# ge.c/fe.c — это штатный crypto-приём (вычитание через дополнение), не баг.
if(MSVC)
    target_compile_options(vtl_ed25519 PRIVATE /wd4146 /wd4244 /wd4267)
endif()
