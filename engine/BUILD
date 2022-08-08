load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "engine",
    srcs = ["engine.cpp"],
    hdrs = ["engine.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//css",
        "//dom",
        "//html",
        "//layout",
        "//protocol",
        "//style",
        "//uri",
        "@spdlog",
    ],
)

cc_test(
    name = "engine_test",
    size = "small",
    srcs = ["engine_test.cpp"],
    deps = [
        ":engine",
        "//etest",
        "//protocol",
        "//uri",
    ],
)
