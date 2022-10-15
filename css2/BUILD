load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "css2",
    srcs = glob(
        include = ["*.cpp"],
        exclude = ["*_test.cpp"],
    ),
    hdrs = glob(["*.h"]),
    visibility = ["//visibility:public"],
    deps = [
        "//util:overloaded",
        "//util:string",
    ],
)

[cc_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    deps = [
        ":css2",
        "//etest",
    ],
) for src in glob(["*_test.cpp"])]
