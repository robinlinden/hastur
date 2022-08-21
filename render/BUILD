load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "render",
    srcs = ["render.cpp"],
    hdrs = ["render.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//dom",
        "//gfx",
        "//layout",
        "//style",
        "//util:string",
        "@range-v3",
        "@spdlog",
    ],
)

cc_test(
    name = "render_test",
    size = "small",
    srcs = ["render_test.cpp"],
    deps = [
        ":render",
        "//dom",
        "//etest",
        "//gfx",
        "//layout",
        "//style",
    ],
)
