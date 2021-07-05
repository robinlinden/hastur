load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "http",
    srcs = ["get.cpp"],
    hdrs = ["get.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//util",
        "@asio",
        "@fmt",
    ],
)
