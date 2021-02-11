load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "http",
    hdrs = ["get.h"],
    srcs = ["get.cpp"],
    visibility = ["//visibility:public"],
    deps = [
        "@asio",
        "@fmt",
    ],
)
