load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "xrender",
    srcs = glob(["src/*.c"]) + ["src/Xrenderint.h"],
    hdrs = ["include/X11/extensions/Xrender.h"],
    # TODO(robinlinden): Make nicer.
    copts = ["-Iexternal/xrender+/include/X11/extensions/"],
    linkopts = ["-lX11"],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)
