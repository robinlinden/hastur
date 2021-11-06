load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "util",
    hdrs = [
        "base_parser.h",
        "string.h",
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
    name = "string_test",
    size = "small",
    srcs = ["string_test.cpp"],
    deps = [
        ":util",
        "//etest",
    ],
)
