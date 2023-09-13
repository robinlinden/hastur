load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_library(
    name = "idna",
    hdrs = glob(["*.h"]),
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
