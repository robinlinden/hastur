load("@rules_cc//cc:defs.bzl", "cc_library")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_library(
    name = "net",
    srcs = ["socket.cpp"],
    hdrs = ["socket.h"],
    copts = HASTUR_COPTS + select({
        "@platforms//os:linux": [
            # asio leaks these into our code.
            "-Wno-nonnull",
            "-Wno-shadow",
            "-Wno-undef",
        ],
        "//conditions:default": [],
    }),
    visibility = ["//visibility:public"],
    deps = ["@asio"],
)
