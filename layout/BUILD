load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_library(
    name = "layout",
    srcs = glob(
        include = ["*.cpp"],
        exclude = ["*_test.cpp"],
    ),
    hdrs = glob(["*.h"]),
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//css",
        "//dom",
        "//geom",
        "//style",
        "//type",
        "//type:naive",
        "//util:from_chars",
        "//util:string",
        "@spdlog",
    ],
)

[cc_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    deps = [
        ":layout",
        "//css",
        "//dom",
        "//etest",
        "//geom",
        "//gfx",
        "//style",
        "//type",
        "//util:string",
    ],
) for src in glob(["*_test.cpp"])]
