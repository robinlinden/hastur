load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")

cc_library(
    name = "gfx",
    srcs = ["opengl_painter.cpp"],
    hdrs = glob(["*.h"]),
    visibility = ["//visibility:public"],
    deps = [
        "//geom",
        "@glew",
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
        "@sfml//:window",
    ],
)
