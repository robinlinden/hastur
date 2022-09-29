load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "protocol",
    srcs = glob(
        include = ["*.cpp"],
        exclude = ["*_test.cpp"],
    ),
    hdrs = glob(["*.h"]),
    visibility = ["//visibility:public"],
    deps = [
        "//net",
        "//uri",
        "//util:string",
        "@fmt",
    ],
)

[cc_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    deps = [
        ":protocol",
        "//etest",
    ],
) for src in glob(["*_test.cpp"])]
