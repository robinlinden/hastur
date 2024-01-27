load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "xproto",
    hdrs = glob(["include/X11/**/*.h"]),
    strip_include_prefix = "include/X11/",
    visibility = ["//visibility:public"],
)

filegroup(
    name = "keysymdef",
    srcs = ["include/X11/keysymdef.h"],
    visibility = ["//visibility:public"],
)
