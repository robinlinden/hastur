load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

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

cc_test(
    name = "protocol_test",
    size = "small",
    srcs = ["get_test.cpp"],
    deps = [
        ":protocol",
        "//etest",
    ],
)
