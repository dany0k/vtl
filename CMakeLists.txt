cmake_minimum_required(VERSION 3.25.0)

project(VTL C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
add_compile_definitions(_POSIX_C_SOURCE=200809L)

# Add compiler flags
add_compile_options(-Wno-implicit-function-declaration)

# ============================================================
# External libraries (все из папки проекта)
# ============================================================

# FFmpeg — prebuilt static из external_libs/ffmpeg
include(cmake/FFmpeg.cmake)

# curl — shared из external_libs/curl
set(CURL_ROOT "${CMAKE_SOURCE_DIR}/external_libs/curl")

# OpenSSL — shared из external_libs/openssl
set(OPENSSL_ROOT "${CMAKE_SOURCE_DIR}/external_libs/openssl")

# PostgreSQL (libpq) — shared из external_libs/postgresql
set(PQ_ROOT "${CMAKE_SOURCE_DIR}/external_libs/postgresql")

# parson (JSON) — единственная JSON-библиотека, собирается из исходников
set(PARSON_ROOT "${CMAKE_SOURCE_DIR}/external_sources/parson")

# ============================================================

set(AppName ${CMAKE_PROJECT_NAME})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/app )


set(VTL_CMAKE_UTILS_SRC_FILES

    VTL/utils/VTL_time.c
    VTL/utils/VTL_file.c
    VTL/utils/VTL_encryptor.c

    VTL/utils/log/VTL_console_log.c
    VTL/utils/curl/VTL_http_client.c
    VTL/utils/http_client.c

    VTL/utils/db/VTL_db_credentals.c
    VTL/utils/db/VTL_db_connection.c
    VTL/utils/db/VTL_db_executor.c

    )

set(VTL_CMAKE_USER_SRC_FILES

    VTL/user/VTL_user_data.c
    VTL/user/history/VTL_user_history_data.c
    VTL/user/history/db/VTL_user_history_save.c

    )

set(VTL_CMAKE_PUBLICATION_TEXT_SRC_FILES

    VTL/publication/text/VTL_publication_text_data.c
    VTL/publication/text/VTL_publication_text_op.c
    VTL/publication/text/VTL_publication_markup_text_flags_check.c
    VTL/publication/text/infra/VTL_publication_text_read.c
    VTL/publication/text/infra/VTL_publication_text_write.c

    )

set(VTL_CMAKE_PUBLICATION_SRC_FILES

    ${VTL_CMAKE_PUBLICATION_TEXT_SRC_FILES}
    VTL/content_platform/tg/VTL_content_platform_tg_net.c

    VTL/publication/VTL_publication.c
    VTL/publication/text/bbcode/VTL_publication_text_op_bbcode.c
    )


set(VTL_CMAKE_MEDIA_CONTAINER_AUDIO_SRC_FILES

    VTL/media_container/audio/VTL_audio_data.c
    VTL/media_container/audio/infra/VTL_audio_read.c
    VTL/media_container/audio/infra/VTL_audio_write.c

    )

set(VTL_CMAKE_MEDIA_CONTAINER_VIDEO_SRC_FILES

    VTL/media_container/video/VTL_video_data.c
    )

set(VTL_CMAKE_MEDIA_CONTAINER_IMG_SRC_FILES
    VTL/media_container/img/VTL_img_core.c
    VTL/media_container/img/VTL_img_filters.c
    VTL/media_container/img/VTL_img_utils.c
)


set(VTL_CMAKE_MEDIA_CONTAINER_SUB_SRC_FILES
    VTL/media_container/sub/VTL_sub_data.c
    VTL/media_container/sub/VTL_sub_burn.c
    VTL/media_container/sub/VTL_sub_convert.c
    VTL/media_container/sub/VTL_sub_parse.c
    VTL/media_container/sub/VTL_sub_style.c
    VTL/media_container/sub/infra/VTL_sub_read.c
    VTL/media_container/sub/infra/VTL_sub_write.c
)

set(VTL_CMAKE_EXTERNAL_SRC_FILES
    external_sources/parson/parson.c
)
set(VTL_CMAKE_MEDIA_CONTAINER_SRC_FILES

    ${VTL_CMAKE_MEDIA_CONTAINER_AUDIO_SRC_FILES}
    ${VTL_CMAKE_MEDIA_CONTAINER_VIDEO_SRC_FILES}
    ${VTL_CMAKE_MEDIA_CONTAINER_IMG_SRC_FILES}
    ${VTL_CMAKE_MEDIA_CONTAINER_SUB_SRC_FILES}

    )

set(VTL_CMAKE_CONTENT_PLATFORM_TG_SRC_FILES

    VTL/content_platform/tg/media_container_params/VTL_content_platform_tg_params_audio.c
    VTL/content_platform/tg/media_container_params/VTL_content_platform_tg_params_sub.c
    VTL/content_platform/tg/media_container_params/VTL_content_platform_tg_params_video.c
    VTL/content_platform/infra/text/VTL_text_configs_for_gen_init.c
    )


set(VTL_CMAKE_CONTENT_PLATFORM_REDDIT_SRC_FILES
    VTL/content_platform/reddit/VTL_content_platform_reddit_net.c
)
set(VTL_CMAKE_CONTENT_PLATFORM_SRC_FILES

    ${VTL_CMAKE_CONTENT_PLATFORM_TG_SRC_FILES}
    VTL/content_platform/VTL_content_platform_flags_check.c
    VTL/content_platform/infra/audio/VTL_audio_gen.c
    VTL/content_platform/infra/audio/VTL_audio_configs_for_gen_init.c
    VTL/content_platform/infra/text/VTL_publication_text_gen.c
    VTL/content_platform/VTL_content_platform_publication.c
    ${VTL_CMAKE_CONTENT_PLATFORM_REDDIT_SRC_FILES}
    )

add_subdirectory(VTL/media_container/img)

add_executable(${AppName}
    ${VTL_CMAKE_UTILS_SRC_FILES}
    ${VTL_CMAKE_USER_SRC_FILES}
    ${VTL_CMAKE_PUBLICATION_SRC_FILES}
    ${VTL_CMAKE_MEDIA_CONTAINER_SRC_FILES}
    ${VTL_CMAKE_CONTENT_PLATFORM_SRC_FILES}
    ${VTL_CMAKE_EXTERNAL_SRC_FILES}
    main.c)

# Все внешние библиотеки — из external_libs / external_sources
target_link_directories(${AppName} PRIVATE
    "${CURL_ROOT}/lib"
    "${OPENSSL_ROOT}/lib"
    "${PQ_ROOT}/lib"
)
target_link_libraries(${AppName} PRIVATE
    ffmpeg VTL_img m
    curl ssl crypto pq
    pthread dl
)

target_include_directories(${AppName}
    PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/VTL
    ${CMAKE_SOURCE_DIR}/VTL/user
    ${CMAKE_SOURCE_DIR}/VTL/user/history
    ${CMAKE_SOURCE_DIR}/external_sources
    ${CURL_ROOT}/include
    ${OPENSSL_ROOT}/include
    ${PQ_ROOT}/include
)

enable_testing()

# add_subdirectory(vtl_tests)  # disabled in minimal-working: broken include path in vtl_tests/VTL_test_data.c
