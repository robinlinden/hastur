load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "net",
    srcs = ["socket.cpp"],
    hdrs = ["socket.h"],
    visibility = ["//visibility:public"],
    deps = ["@asio"],
)
