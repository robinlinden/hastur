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
    implementation_deps = [
        "//util:from_chars",
        "//util:string",
        "@spdlog",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//css",
        "//dom",
        "//geom",
        "//style",
        "//type",
        "//type:naive",
    ],
)

[cc_test(
    name = src.removesuffix(".cpp"),
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
        "//type:naive",
        "//util:string",
    ],
) for src in glob(["*_test.cpp"])]
