load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_library(
    name = "azm",
    hdrs = ["assembler.h"],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
)

[cc_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    deps = [
        ":azm",
        "//etest",
    ],
) for src in glob(
    include = ["*_test.cpp"],
)]

cc_binary(
    name = "azm_example",
    srcs = ["azm_example.cpp"],
    copts = HASTUR_COPTS,
    deps = [
        ":azm",
    ],
)
