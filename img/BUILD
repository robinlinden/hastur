load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "img",
    srcs = glob(
        include = ["*.cpp"],
        exclude = ["*_test.cpp"],
    ),
    hdrs = glob(["*.h"]),
    visibility = ["//visibility:public"],
    deps = [
        "@libpng",
    ],
)

# See: https://www.mjt.me.uk/posts/smallest-png/
genrule(
    name = "tiny_png",
    srcs = ["tiny.png"],
    outs = ["tiny_png.h"],
    cmd = "xxd -i $< >$@",
)

extra_srcs = {
    "png": [":tiny_png"],
}

[cc_test(
    name = src[:-4],
    size = "small",
    srcs = [src] + extra_srcs.get(
        src[:-9],
        [],
    ),
    deps = [
        ":img",
        "//etest",
    ],
) for src in glob(["*_test.cpp"])]
