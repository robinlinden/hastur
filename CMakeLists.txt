cmake_minimum_required(VERSION 3.18)

project(hastur)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

find_package(Asio REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)

# css
add_library(hastur_css
    css/parse.cpp
    css/parse.h
    css/parser.h
    css/rule.h
)

target_include_directories(hastur_css PUBLIC .)
target_link_libraries(hastur_css
    PUBLIC
        hastur_util
)

target_compile_features(hastur_css PUBLIC cxx_std_20)
set_target_properties(hastur_css
    PROPERTIES
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

# dom
add_library(hastur_dom
    dom/dom.cpp
    dom/dom.h
)

target_include_directories(hastur_dom PUBLIC .)
target_compile_features(hastur_dom PUBLIC cxx_std_20)
set_target_properties(hastur_dom
    PROPERTIES
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

# html
add_library(hastur_html
    html/parse.cpp
    html/parse.h
    html/parser.h
)

target_include_directories(hastur_html PUBLIC .)
target_link_libraries(hastur_html
    PUBLIC
        hastur_dom
        hastur_util
)

target_compile_features(hastur_html PUBLIC cxx_std_20)
set_target_properties(hastur_html
    PROPERTIES
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

# http
add_library(hastur_http
    http/get.cpp
    http/get.h
)

target_include_directories(hastur_http PUBLIC .)
target_link_libraries(hastur_http
    PRIVATE
        Asio::Asio
        fmt::fmt
        spdlog::spdlog
)

target_compile_features(hastur_http PUBLIC cxx_std_20)
set_target_properties(hastur_http
    PROPERTIES
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

# style
add_library(hastur_style
    style/style.cpp
    style/style.h
    style/styled_node.h
)

target_include_directories(hastur_style PUBLIC .)
target_link_libraries(hastur_style
    PRIVATE
        hastur_css
        hastur_dom
)

target_compile_features(hastur_style PUBLIC cxx_std_20)
set_target_properties(hastur_style
    PROPERTIES
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

# util
add_library(hastur_util INTERFACE)
target_include_directories(hastur_util INTERFACE .)
target_compile_features(hastur_util INTERFACE cxx_std_20)
set_target_properties(hastur_util
    PROPERTIES
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)
