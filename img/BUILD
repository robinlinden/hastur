load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")
load("@rules_fuzzing//fuzzing:cc_defs.bzl", "cc_fuzz_test")
load("//bzl:copts.bzl", "HASTUR_COPTS", "HASTUR_FUZZ_PLATFORMS")

cc_library(
    name = "img",
    srcs = glob(
        include = ["*.cpp"],
        exclude = [
            "*_example.cpp",
            "*_test.cpp",
        ],
    ),
    hdrs = glob(["*.h"]),
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "@expected",
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
    copts = HASTUR_COPTS,
    deps = [
        ":img",
        "//etest",
        "@expected",
    ],
) for src in glob(
    include = ["*_test.cpp"],
    exclude = ["*_fuzz_test.cpp"],
)]

[cc_fuzz_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    target_compatible_with = HASTUR_FUZZ_PLATFORMS,
    deps = [":img"],
) for src in glob(["*_fuzz_test.cpp"])]

cc_binary(
    name = "img_example",
    srcs = ["img_example.cpp"],
    copts = HASTUR_COPTS,
    deps = [
        ":img",
        "@sfml//:graphics",
        "@sfml//:window",
    ],
)
