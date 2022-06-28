load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "layout",
    srcs = ["layout.cpp"],
    hdrs = glob(["*.h"]),
    visibility = ["//visibility:public"],
    deps = [
        "//geom",
        "//style",
    ],
)

[cc_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    deps = [
        ":layout",
        "//etest",
    ],
) for src in glob(["*_test.cpp"])]
