load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "util",
    srcs = ["uri.cpp"],
    hdrs = [
        "base_parser.h",
        "uri.h",
    ],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "base_parser_test",
    size = "small",
    srcs = ["base_parser_test.cpp"],
    deps = [
        ":util",
        "//etest",
    ],
)

cc_test(
    name = "uri_test",
    size = "small",
    srcs = ["uri_test.cpp"],
    deps = [
        ":util",
        "//etest",
    ],
)
