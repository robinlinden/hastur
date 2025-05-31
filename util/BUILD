load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

dependencies = {
    "arg_parser": ["@expected"],
}

[cc_library(
    name = hdr.removesuffix(".h"),
    hdrs = [hdr],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = dependencies.get(
        hdr.removesuffix(".h"),
        [],
    ),
) for hdr in glob(["*.h"])]

[cc_test(
    name = src.removesuffix(".cpp"),
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    deps = [
        ":%s" % src.removesuffix("_test.cpp"),
        "//etest",
    ],
) for src in glob(["*_test.cpp"])]
