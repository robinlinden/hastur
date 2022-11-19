load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")
load("//bzl:defs.bzl", "cc_xfail_test")

cc_library(
    name = "etest",
    srcs = ["etest.cpp"],
    hdrs = [
        "cxx_compat.h",
        "etest.h",
    ],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
)

[cc_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    deps = [":etest"],
) for src in glob(
    include = ["*_test.cpp"],
    exclude = ["*_failure_test.cpp"],
)]

[cc_xfail_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    deps = [":etest"],
) for src in glob(["*_failure_test.cpp"])]
