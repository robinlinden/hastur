load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "js",
    hdrs = glob(["*.h"]),
    visibility = ["//visibility:public"],
)

[cc_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    deps = [
        ":js",
        "//etest",
    ],
) for src in glob(["*_test.cpp"])]
