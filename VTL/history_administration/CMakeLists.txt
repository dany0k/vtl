add_library(VTL_history_administration OBJECT
    VTL_history_administration_data.c
    db/VTL_history_administration_db_create.c
    db/VTL_history_administration_db_read.c
    db/VTL_history_administration_db_edit.c
    db/VTL_history_administration_db_delete.c
    io/VTL_history_administration_io_show.c
)

target_include_directories(VTL_history_administration PUBLIC ${CMAKE_SOURCE_DIR})

target_link_libraries(VTL_history_administration PUBLIC
    VTL_user
    VTL_utils
    PostgreSQL::PostgreSQL
)

set_target_properties(VTL_history_administration PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

if(MSVC)
    target_compile_options(VTL_history_administration PRIVATE /W4 /utf-8)
else()
    target_compile_options(VTL_history_administration PRIVATE
        -Wall -Wextra
        -Wno-error=implicit-function-declaration
        -Wno-error=incompatible-pointer-types
        -Wno-error=int-conversion
    )
endif()
