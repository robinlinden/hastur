# Asio_FOUND
# Asio::Asio

find_path(Asio_INCLUDE_DIR NAMES asio.hpp)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Asio
    DEFAULT_MSG
    Asio_INCLUDE_DIR
)

mark_as_advanced(Asio_INCLUDE_DIR)

if(Asio_FOUND)
    find_package(Threads QUIET REQUIRED)

    add_library(Asio::Asio INTERFACE IMPORTED)
    set_target_properties(Asio::Asio
        PROPERTIES
            INTERFACE_COMPILE_DEFINITIONS ASIO_STANDALONE
            INTERFACE_INCLUDE_DIRECTORIES ${Asio_INCLUDE_DIR}
    )
    target_link_libraries(Asio::Asio INTERFACE Threads::Threads)
endif()
