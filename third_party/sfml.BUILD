load("@rules_cc//cc:defs.bzl", "cc_library")

SFML_DEFINES = [
    "SFML_STATIC",
    "UNICODE",
    "_UNICODE",
]

cc_library(
    name = "system",
    srcs = glob([
        "src/SFML/System/*.cpp",
        "src/SFML/System/*.hpp",
    ]) + select({
        "@platforms//os:linux": glob([
            "src/SFML/System/Unix/**/*.cpp",
            "src/SFML/System/Unix/**/*.hpp",
        ]),
        "@platforms//os:windows": glob([
            "src/SFML/System/Win32/**/*.cpp",
            "src/SFML/System/Win32/**/*.hpp",
        ]),
    }),
    hdrs = glob([
        "include/SFML/*",
        "include/SFML/System/*",
    ]),
    copts = ["-Iexternal/sfml/src/"],
    defines = SFML_DEFINES,
    linkopts = select({
        "@platforms//os:linux": [
            "-ludev",
            "-pthread",
        ],
        "@platforms//os:windows": [
            "-DEFAULTLIB:winmm",
        ],
    }),
    strip_include_prefix = "include/",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "window",
    srcs = glob(
        include = [
            "src/SFML/Window/*.cpp",
            "src/SFML/Window/*.hpp",
        ],
        exclude = [
            "src/SFML/Window/EGLCheck.cpp",
            "src/SFML/Window/EGLCheck.hpp",
            "src/SFML/Window/EglContext.cpp",
            "src/SFML/Window/EglContext.hpp",
        ],
    ) + select({
        "@platforms//os:linux": glob([
            "src/SFML/Window/Unix/*.cpp",
            "src/SFML/Window/Unix/*.hpp",
        ]),
        "@platforms//os:windows": glob([
            "src/SFML/Window/Win32/*.cpp",
            "src/SFML/Window/Win32/*.hpp",
        ]),
    }),
    hdrs = glob(["include/SFML/Window/*"]),
    copts = ["-Iexternal/sfml/src/"],
    defines = SFML_DEFINES,
    linkopts = select({
        "@platforms//os:linux": [
            "-lGL",
            "-lX11",
            "-lXrandr",
            "-ludev",
        ],
        "@platforms//os:windows": [
            "-DEFAULTLIB:advapi32",
            "-DEFAULTLIB:gdi32",
            "-DEFAULTLIB:opengl32",
            "-DEFAULTLIB:user32",
            "-DEFAULTLIB:winmm",
        ],
    }),
    strip_include_prefix = "include/",
    visibility = ["//visibility:public"],
    deps = [":system"],
)

# TODO(robinlinden): Don't use checked-in libraries.
cc_library(
    name = "freetype2",
    srcs = select({
        "@platforms//os:linux": [],
        "@platforms//os:windows": ["extlibs/libs-msvc/x64/freetype.lib"],
    }),
    hdrs = glob(["extlibs/headers/freetype2/**/*.h"]),
    linkopts = select({
        "@platforms//os:linux": ["-lfreetype"],
        "@platforms//os:windows": [],
    }),
    strip_include_prefix = "extlibs/headers/freetype2/",
)

cc_library(
    name = "graphics",
    srcs = glob(
        include = [
            "src/SFML/Graphics/*.cpp",
            "src/SFML/Graphics/*.hpp",
        ],
    ) + select({
        "@platforms//os:linux": glob([
            "src/SFML/Graphics/Unix/*.cpp",
            "src/SFML/Graphics/Unix/*.hpp",
        ]),
        "@platforms//os:windows": glob([
            "src/SFML/Graphics/Win32/*.cpp",
            "src/SFML/Graphics/Win32/*.hpp",
        ]),
    }),
    hdrs = glob(["include/SFML/Graphics/*"]),
    copts = ["-Iexternal/sfml/src/"],
    defines = SFML_DEFINES,
    linkopts = select({
        "@platforms//os:linux": [
            "-lGL",
            "-lX11",
        ],
        "@platforms//os:windows": [],
    }),
    strip_include_prefix = "include/",
    visibility = ["//visibility:public"],
    deps = [
        ":freetype2",
        ":window",
        "@stb//:image",
        "@stb//:image_write",
    ],
)
