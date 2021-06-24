load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "zlib",
    srcs = glob([
        "*.c",
        "*.h",
    ]),
    hdrs = ["zlib.h"],
    local_defines = select({
        "@platforms//os:linux": ["Z_HAVE_UNISTD_H"],
        "@platforms//os:windows": [],
    }),
    visibility = ["//visibility:public"],
)
