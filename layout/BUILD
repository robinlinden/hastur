load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "layout",
    srcs = ["layout.cpp"],
    hdrs = ["layout.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//geom",
        "//style",
    ],
)

cc_test(
    name = "layout_test",
    size = "small",
    srcs = ["layout_test.cpp"],
    deps = [
        ":layout",
        "//etest",
    ],
)
