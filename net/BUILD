load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

NET_COPTS = HASTUR_COPTS + select({
    "@platforms//os:linux": [
        # asio leaks this into our code.
        "-Wno-null-dereference",
        "-Wno-shadow",
        "-Wno-unknown-pragmas",
    ],
    "@platforms//os:macos": [
        "-Wno-null-dereference",
        "-Wno-shadow",
        "-Wno-unknown-pragmas",
    ],
    "//conditions:default": [],
})

cc_library(
    name = "net",
    srcs = glob(
        include = ["*.cpp"],
        exclude = ["*_test.cpp"],
    ),
    hdrs = glob(["*.h"]),
    copts = NET_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "@asio",
        "@boringssl//:ssl",
    ],
)

[cc_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    copts = NET_COPTS,
    deps = [
        ":net",
        "//etest",
        "@asio",
    ],
) for src in glob(["*_test.cpp"])]
