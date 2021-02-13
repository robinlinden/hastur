load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "fmt",
    srcs = glob(["src/*.cc"]),
    hdrs = glob(["include/**/*.h"]),
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "header_only",
    hdrs = glob(["include/**/*.h"]),
    defines = ["FMT_HEADER_ONLY=1"],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)
