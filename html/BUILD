load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_library(
    name = "html",
    hdrs = glob(["*.h"]),
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//dom",
        "//html2",
    ],
)

[cc_test(
    name = src.removesuffix(".cpp"),
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    deps = [
        ":html",
        "//dom",
        "//etest",
        "//html2",
    ],
) for src in glob(["*_test.cpp"])]
