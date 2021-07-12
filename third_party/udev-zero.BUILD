load("@rules_cc//cc:defs.bzl", "cc_library")

genrule(
    name = "generate_public_header",
    srcs = ["udev.h"],
    outs = ["libudev.h"],
    cmd = "cp $< $@",
)

cc_library(
    name = "udev-zero",
    srcs = glob([
        "*.c",
        "*.h",
    ]),
    hdrs = [":generate_public_header"],
    includes = ["."],
    target_compatible_with = ["@platforms//os:linux"],
    visibility = ["//visibility:public"],
)
