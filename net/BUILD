load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_library(
    name = "net",
    srcs = glob(
        include = ["*.cpp"],
        exclude = ["*_test.cpp"],
    ),
    hdrs = glob(["*.h"]),
    copts = HASTUR_COPTS + select({
        "@platforms//os:linux": [
            # asio leaks this into our code.
            "-Wno-shadow",
        ],
        "//conditions:default": [],
    }),
    visibility = ["//visibility:public"],
    deps = [
        "@asio",
        "@fmt",
    ],
)

cc_test(
    name = "ip_test",
    size = "small",
    srcs = ["ip_test.cpp"],
    copts = HASTUR_COPTS,
    deps = [
        ":net",
        "//etest",
    ],
)
