load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_library(
    name = "html",
    srcs = ["parser.cpp"],
    hdrs = ["parser.h"],
    copts = HASTUR_COPTS,
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
    copts = HASTUR_COPTS,
    deps = [
        ":html",
        "//etest",
    ],
)
