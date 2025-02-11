load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_library(
    name = "render",
    srcs = ["render.cpp"],
    hdrs = ["render.h"],
    copts = HASTUR_COPTS,
    implementation_deps = [
        "//css",
        "//dom",
        "//style",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//geom",
        "//gfx",
        "//layout",
    ],
)

cc_test(
    name = "render_test",
    size = "small",
    srcs = ["render_test.cpp"],
    copts = HASTUR_COPTS,
    deps = [
        ":render",
        "//css",
        "//dom",
        "//etest",
        "//geom",
        "//gfx",
        "//layout",
        "//style",
    ],
)
