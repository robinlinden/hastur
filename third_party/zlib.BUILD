load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "zlib",
    srcs = glob([
        "*.c",
        "*.h",
    ]),
    hdrs = [
        "zconf.h",
        "zlib.h",
    ],
    defines = ["ZLIB_CONST"],
    includes = ["."],
    local_defines = select({
        "@platforms//os:linux": ["Z_HAVE_UNISTD_H"],
        "@platforms//os:macos": ["Z_HAVE_UNISTD_H"],
        "@platforms//os:windows": [],
    }),
    target_compatible_with = select({
        "@platforms//os:wasi": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    visibility = ["//visibility:public"],
)
