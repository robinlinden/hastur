load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "common",
    srcs = glob([
        "source/common/*.h",
        "source/common/*.cpp",
        "source/stubdata/*.h",
        "source/stubdata/*.cpp",
    ]),
    hdrs = glob([
        "source/common/unicode/*.h",
    ]),
    copts = select({
        "@platforms//os:windows": [
            "/GR",
            "-utf-8",
            "-I source/common/",
            "-I source/common/unicode/",
            "-I source/stubdata/",
        ],
        "//conditions:default": [
            "-frtti",
            "-I source/common/",
            "-I source/common/unicode/",
            "-I source/stubdata/",
        ],
    }) + select({
        "@bazel_tools//tools/cpp:clang-cl": [
            "-Wno-microsoft-include",
        ],
        "//conditions:default": [],
    }),
    linkopts = select({
        "@platforms//os:windows": [],
        "//conditions:default": ["-ldl"],
    }),
    local_defines = [
        "U_COMMON_IMPLEMENTATION",
    ],
    strip_include_prefix = "source/common/",
    visibility = ["//visibility:public"],
)
