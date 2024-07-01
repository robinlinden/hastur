load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("@rules_python//python:defs.bzl", "py_binary")
load("//bzl:copts.bzl", "HASTUR_COPTS")

py_binary(
    name = "unicode_data_processor",
    srcs = ["unicode_data_processor.py"],
)

genrule(
    name = "generate_unicode_data",
    srcs = ["@ucd//:UnicodeData.txt"],
    outs = ["unicode_data.h"],
    cmd = "$(location :unicode_data_processor) $(location @ucd//:UnicodeData.txt) >$@",
    tools = [":unicode_data_processor"],
)

cc_library(
    name = "normalization",
    srcs = [
        "normalization.cpp",
        ":generate_unicode_data",
    ],
    hdrs = ["normalization.h"],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = [":util"],
)

cc_library(
    name = "util",
    hdrs = ["util.h"],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
)

[cc_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    deps = [
        ":normalization",
        ":util",
        "//etest",
    ],
) for src in glob(["*_test.cpp"])]
