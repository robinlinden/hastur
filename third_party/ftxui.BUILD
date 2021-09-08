load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "ftxui",
    srcs = glob(
        include = ["src/**"],
        exclude = [
            "src/**/*_test.cpp",
            "src/**/*_fuzzer.cpp",
        ],
    ),
    hdrs = glob(["include/**/*.hpp"]),
    copts = ["-Iexternal/ftxui/src"] + select({
        "@platforms//os:linux": [],
        "@platforms//os:windows": ["-utf-8"],
    }),
    local_defines = select({
        "@platforms//os:linux": [],
        "@platforms//os:windows": [
            "_UNICODE",
            "UNICODE",
        ],
    }),
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)
