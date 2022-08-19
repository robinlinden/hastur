load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "uri",
    srcs = ["uri.cpp"],
    hdrs = ["uri.h"],
    visibility = ["//visibility:public"],
    deps = ["//util:string"],
)

cc_test(
    name = "uri_test",
    size = "small",
    srcs = ["uri_test.cpp"],
    deps = [
        ":uri",
        "//etest",
    ],
)
