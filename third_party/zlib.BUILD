load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "zlib",
    srcs = glob([
        "*.c",
        "*.h",
    ]),
    hdrs = ["zlib.h"],
    defines = ["ZLIB_CONST"],
    includes = ["."],
    local_defines = select({
        "@platforms//os:linux": ["Z_HAVE_UNISTD_H"],
        "@platforms//os:macos": ["Z_HAVE_UNISTD_H"],
        "@platforms//os:windows": [],
    }),
    visibility = ["//visibility:public"],
)
