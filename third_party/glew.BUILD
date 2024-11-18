load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_library(
    name = "glew",
    srcs = ["src/glew.c"],
    hdrs = glob(["include/**/*.h"]),
    defines = [
        "GLEW_NO_GLU",
        "GLEW_STATIC",
    ],
    includes = ["include/"],
    linkopts = select({
        "@platforms//os:linux": [
            "-lGL",
            "-lX11",
        ],
        "@platforms//os:windows": ["-DEFAULTLIB:opengl32"],
    }),
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "glewinfo",
    srcs = ["src/glewinfo.c"],
    linkopts = select({
        "@platforms//os:linux": [],
        "@platforms//os:windows": [
            "-DEFAULTLIB:Gdi32.lib",
            "-DEFAULTLIB:User32.lib",
        ],
    }),
    deps = [":glew"],
)
