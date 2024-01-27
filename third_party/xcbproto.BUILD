load("@rules_python//python:defs.bzl", "py_library")

py_library(
    name = "xcbgen",
    srcs = glob(["xcbgen/*.py"]),
    visibility = ["//visibility:public"],
)

filegroup(
    name = "bigreq",
    srcs = ["src/bigreq.xml"],
    visibility = ["//visibility:public"],
)

filegroup(
    name = "xproto",
    srcs = ["src/xproto.xml"],
    visibility = ["//visibility:public"],
)

filegroup(
    name = "xc_misc",
    srcs = ["src/xc_misc.xml"],
    visibility = ["//visibility:public"],
)
