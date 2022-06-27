load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "spdlog",
    srcs = glob(
        include = ["src/*.cpp"],
        exclude = ["src/fmt.cpp"],
    ),
    hdrs = glob(["include/**/*.h"]),
    defines = [
        "SPDLOG_COMPILED_LIB",
        "SPDLOG_FMT_EXTERNAL",
        "SPDLOG_NO_EXCEPTIONS",
    ],
    linkopts = select({
        "@platforms//os:linux": ["-lpthread"],
        "@platforms//os:windows": [],
    }),
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
    deps = ["@fmt"],
)

cc_library(
    name = "header_only",
    hdrs = glob(["include/**/*.h"]),
    linkopts = select({
        "@platforms//os:linux": ["-lpthread"],
        "@platforms//os:windows": [],
    }),
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
    deps = ["@fmt"],
)
