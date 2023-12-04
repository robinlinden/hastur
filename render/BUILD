load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_library(
    name = "render",
    srcs = ["render.cpp"],
    hdrs = ["render.h"],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//css",
        "//gfx",
        "//layout",
        "@spdlog",
    ],
)

cc_test(
    name = "render_test",
    size = "small",
    srcs = ["render_test.cpp"],
    copts = HASTUR_COPTS,
    deps = [
        ":render",
        "//dom",
        "//etest",
        "//gfx",
        "//layout",
        "//style",
    ],
)
