load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_library(
    name = "archive",
    srcs = glob(
        include = ["*.cpp"],
        exclude = [
            "*_test.cpp",
        ],
    ),
    hdrs = glob(["*.h"]),
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "@expected",
        "@zlib",
    ],
)

[cc_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    deps = [
        ":archive",
        "//etest",
    ],
) for src in glob(["*_test.cpp"])]
