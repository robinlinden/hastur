load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

genrule(
    name = "generate_pnglibconf",
    srcs = ["scripts/pnglibconf.h.prebuilt"],
    outs = ["pnglibconf.h"],
    cmd = "cp $< $@",
)

cc_library(
    name = "libpng",
    srcs = glob(
        include = [
            "*.c",
            "*.h",
        ],
        exclude = [
            "example.c",
            "pngtest.c",
        ],
    ) + [":generate_pnglibconf"],
    hdrs = [
        "png.h",
        "pngconf.h",
    ],
    includes = ["."],
    visibility = ["//visibility:public"],
    deps = ["@zlib"],
)

cc_test(
    name = "pngtest",
    size = "small",
    srcs = ["pngtest.c"],
    args = ["$(location :pngtest.png)"],
    data = ["pngtest.png"],
    deps = [":libpng"],
)
