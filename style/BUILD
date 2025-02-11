load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_library(
    name = "style",
    srcs = glob(
        include = ["*.cpp"],
        exclude = [
            "*_test.cpp",
            "*_bench.cpp",
        ],
    ),
    hdrs = glob(["*.h"]),
    copts = HASTUR_COPTS,
    implementation_deps = [
        "//util:from_chars",
        "@spdlog",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//css",
        "//dom",
        "//gfx",
        "//util:string",
    ],
)

[cc_test(
    name = src.removesuffix(".cpp"),
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    deps = [
        ":style",
        "//css",
        "//dom",
        "//etest",
        "//gfx",
    ],
) for src in glob(["*_test.cpp"])]

[cc_test(
    name = src.removesuffix(".cpp"),
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    deps = [
        ":style",
        "//dom",
        "//etest",
        "@nanobench",
    ],
) for src in glob(["*_bench.cpp"])]
