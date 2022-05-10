load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")

cc_library(
    name = "gfx",
    hdrs = glob(
        include = ["*.h"],
        exclude = ["*_painter.h"],
    ),
    visibility = ["//visibility:public"],
    deps = ["//geom"],
)

cc_library(
    name = "opengl",
    srcs = ["opengl_painter.cpp"],
    hdrs = ["opengl_painter.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":gfx",
        "@glew",
    ],
)

cc_library(
    name = "sfml",
    srcs = ["sfml_painter.cpp"],
    hdrs = ["sfml_painter.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":gfx",
        "//os",
        "@sfml//:graphics",
    ],
)

[cc_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    deps = [
        ":gfx",
        "//etest",
    ],
) for src in glob(["*_test.cpp"])]

cc_binary(
    name = "gfx_example",
    srcs = ["gfx_example.cpp"],
    deps = [
        ":gfx",
        ":opengl",
        ":sfml",
        "@sfml//:graphics",
        "@sfml//:window",
    ],
)
