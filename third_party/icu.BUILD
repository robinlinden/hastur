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
    defines = [
        "U_STATIC_IMPLEMENTATION",
        "U_COMMON_IMPLEMENTATION",
        "U_CHARSET_IS_UTF8=1",
        "U_HIDE_OBSOLETE_UTF_OLD_H=1",
        "UCONFIG_NO_CONVERSION=1",
    ],
    linkopts = select({
        "@platforms//os:windows": [
            "-DEFAULTLIB:advapi32",
        ],
        "//conditions:default": ["-ldl"],
    }),
    linkstatic = True,
    strip_include_prefix = "source/common/",
    visibility = ["//visibility:public"],
)
