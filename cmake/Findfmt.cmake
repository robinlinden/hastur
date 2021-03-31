# fmt_FOUND
# fmt::fmt

find_path(fmt_INCLUDE_DIR NAMES fmt/format.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    fmt
    DEFAULT_MSG
    fmt_INCLUDE_DIR
)

mark_as_advanced(fmt_INCLUDE_DIR)

if(fmt_FOUND)
    add_library(fmt::fmt INTERFACE IMPORTED)
    set_target_properties(fmt::fmt
        PROPERTIES
            INTERFACE_COMPILE_DEFINITIONS FMT_HEADER_ONLY
            INTERFACE_INCLUDE_DIRECTORIES ${fmt_INCLUDE_DIR}
    )
endif()
