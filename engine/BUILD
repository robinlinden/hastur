load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_library(
    name = "engine",
    srcs = ["engine.cpp"],
    hdrs = ["engine.h"],
    copts = HASTUR_COPTS,
    implementation_deps = [
        "//archive:brotli",
        "//archive:zlib",
        "//archive:zstd",
        "//html",
        "//js",
        "//util:string",
        "@spdlog",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//css",
        "//dom",
        "//layout",
        "//protocol",
        "//style",
        "//type",
        "//type:naive",
        "//uri",
        "@expected",
    ],
)

cc_test(
    name = "engine_test",
    size = "small",
    srcs = ["engine_test.cpp"],
    copts = HASTUR_COPTS,
    deps = [
        ":engine",
        "//css",
        "//dom",
        "//etest",
        "//gfx",
        "//protocol",
        "//style",
        "//type",
        "//type:naive",
        "//uri",
        "@expected",
    ],
)
