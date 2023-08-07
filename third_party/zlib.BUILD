load("@rules_cc//cc:defs.bzl", "cc_library")

ZLIB_COPTS = select({
    "@platforms//os:linux": [
        "-Wno-deprecated-non-prototype",
    ],
    "//conditions:default": [],
})

cc_library(
    name = "zlib",
    srcs = glob([
        "*.c",
        "*.h",
    ]),
    hdrs = ["zlib.h"],
    copts = ZLIB_COPTS,
    defines = ["ZLIB_CONST"],
    includes = ["."],
    local_defines = select({
        "@platforms//os:linux": ["Z_HAVE_UNISTD_H"],
        "@platforms//os:windows": [],
    }),
    visibility = ["//visibility:public"],
)
