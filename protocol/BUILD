load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "protocol",
    srcs = ["get.cpp"],
    hdrs = ["get.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//uri",
        "@asio",
        "@fmt",
    ],
)
