load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_library(
    name = "layout",
    srcs = ["layout.cpp"],
    hdrs = glob(["*.h"]),
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//css",
        "//geom",
        "//style",
        "//util:from_chars",
        "//util:string",
        "@spdlog",
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
