load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("@rules_python//python:defs.bzl", "py_binary")
load("//bzl:copts.bzl", "HASTUR_COPTS")

py_binary(
    name = "idna_data_processor",
    srcs = ["idna_data_processor.py"],
)

genrule(
    name = "generate_idna_data",
    srcs = ["@idna_mapping_table//file"],
    outs = ["idna_data.h"],
    cmd = "$(location :idna_data_processor) $(location @idna_mapping_table//file) >$@",
    tools = [":idna_data_processor"],
)

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
    name = "idna",
    srcs = glob(
        include = ["*.cpp"],
        exclude = ["*_test.cpp"],
    ),
    hdrs = glob(["*.h"]) + [
        ":generate_idna_data",
        ":generate_unicode_data",
    ],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = ["//util:unicode"],
)

[cc_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    deps = [
        ":idna",
        "//etest",
        "//util:unicode",
    ],
) for src in glob(["*_test.cpp"])]
