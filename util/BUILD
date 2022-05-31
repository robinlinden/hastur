load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

[cc_library(
    name = hdr[:-2],
    hdrs = [hdr],
    visibility = ["//visibility:public"],
) for hdr in glob(["*.h"])]

[cc_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    deps = [
        ":%s" % src[:-9],
        "//etest",
    ],
) for src in glob(["*_test.cpp"])]
