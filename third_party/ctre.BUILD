load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "ctre",
    hdrs = glob(["include/**/*.hpp"]),
    includes = ["include/"],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)

[cc_library(
    name = src[:-4],
    srcs = [src],
    deps = [":ctre"],
) for src in glob(["tests/*.cpp"])]
