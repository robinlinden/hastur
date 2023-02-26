load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "xext",
    srcs = glob([
        "src/*.c",
        "src/*.h",
    ]),
    hdrs = glob(["include/X11/extensions/*.h"]),
    linkopts = ["-lX11"],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)
