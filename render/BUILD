load("@rules_cc//cc:defs.bzl", "cc_library")

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
        "@spdlog",
    ],
)
