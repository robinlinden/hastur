load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")
load("//bzl:defs.bzl", "cc_xfail_test")

cc_library(
    name = "etest",
    srcs = glob(
        include = ["*.cpp"],
        exclude = ["*_test.cpp"],
    ),
    hdrs = glob(["*.h"]),
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
)

[cc_test(
    name = src.removesuffix(".cpp"),
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    deps = [":etest"],
) for src in glob(
    include = ["*_test.cpp"],
    exclude = ["*_failure_test.cpp"],
)]

[cc_xfail_test(
    name = src.removesuffix(".cpp"),
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    tags = ["no-cross"],
    deps = [":etest"],
) for src in glob(["*_failure_test.cpp"])]
