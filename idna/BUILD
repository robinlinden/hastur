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

cc_library(
    name = "idna",
    srcs = glob(
        include = ["*.cpp"],
        exclude = ["*_test.cpp"],
    ),
    hdrs = glob(["*.h"]) + [
        ":generate_idna_data",
    ],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = ["//unicode:util"],
)

[cc_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    deps = [
        ":idna",
        "//etest",
        "//unicode:util",
    ],
) for src in glob(["*_test.cpp"])]
