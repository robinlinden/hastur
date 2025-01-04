load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "nanobench",
    hdrs = ["src/include/nanobench.h"],
    defines = ["ANKERL_NANOBENCH_IMPLEMENT"],
    strip_include_prefix = "src/include",
    visibility = ["//visibility:public"],
)
