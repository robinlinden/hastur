load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:defs.bzl", "cc_xfail_test")

cc_library(
    name = "etest",
    srcs = ["etest.cpp"],
    hdrs = [
        "cxx_compat.h",
        "etest.h",
    ],
    visibility = ["//visibility:public"],
)

[cc_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    deps = [":etest"],
) for src in glob(
    include = ["*_test.cpp"],
    exclude = ["*_failure_test.cpp"],
)]

[cc_xfail_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    deps = [":etest"],
) for src in glob(["*_failure_test.cpp"])]
