load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "html",
    srcs = [
        "parse.cpp",
        "parser.cpp",
    ],
    hdrs = [
        "parse.h",
        "parser.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//dom",
        "//html2",
        "//util:string",
        "@spdlog",
    ],
)

cc_test(
    name = "html_test",
    size = "small",
    srcs = ["parser_test.cpp"],
    deps = [
        ":html",
        "//etest",
    ],
)
