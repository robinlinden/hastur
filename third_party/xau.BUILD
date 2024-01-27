load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "xau",
    srcs = glob(["*.c"]),
    hdrs = ["include/X11/Xauth.h"],
    strip_include_prefix = "include/X11",
    visibility = ["//visibility:public"],
)
