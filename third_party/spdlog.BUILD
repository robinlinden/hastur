load("@rules_cc//cc:defs.bzl", "cc_library")

PTHREAD_OPTS = select({
    "@platforms//os:linux": ["-pthread"],
    "@platforms//os:macos": ["-pthread"],
    "//conditions:default": [],
})

cc_library(
    name = "spdlog",
    srcs = glob(
        include = ["src/*.cpp"],
        exclude = ["src/fmt.cpp"],
    ),
    hdrs = glob(["include/**/*.h"]),
    copts = PTHREAD_OPTS,
    defines = [
        "SPDLOG_COMPILED_LIB",
        "SPDLOG_FMT_EXTERNAL",
        "SPDLOG_NO_EXCEPTIONS",
        "SPDLOG_USE_STD_FORMAT",
    ],
    linkopts = PTHREAD_OPTS,
    strip_include_prefix = "include",
    target_compatible_with = select({
        "@platforms//os:wasi": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    visibility = ["//visibility:public"],
)
