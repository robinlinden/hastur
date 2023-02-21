load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

dependencies = {
    "base_parser": [":string"],
}

[cc_library(
    name = hdr[:-2],
    hdrs = [hdr],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = dependencies.get(
        hdr[:-2],
        [],
    ),
) for hdr in glob(["*.h"])]

[cc_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    deps = [
        ":%s" % src[:-9],
        "//etest",
    ],
) for src in glob(["*_test.cpp"])]
