load("@rules_cc//cc:defs.bzl", "cc_library")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_library(
    name = "archive",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.h"]),
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "@expected",
        "@zlib",
    ],
)
