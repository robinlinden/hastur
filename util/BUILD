load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "base_parser",
    hdrs = ["base_parser.h"],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "base_parser_test",
    size = "small",
    srcs = ["base_parser_test.cpp"],
    deps = [
        ":base_parser",
        "//etest",
    ],
)

cc_library(
    name = "string",
    hdrs = ["string.h"],
    visibility = ["//visibility:public"],
    deps = ["@range-v3"],
)

cc_test(
    name = "string_test",
    size = "small",
    srcs = ["string_test.cpp"],
    deps = [
        ":string",
        "//etest",
    ],
)
