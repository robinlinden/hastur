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
    local_defines = ["PNG_ARM_NEON_OPT=0"],
    target_compatible_with = select({
        "@platforms//os:wasi": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    visibility = ["//visibility:public"],
    deps = ["@zlib"],
)

cc_test(
    name = "pngtest",
    size = "small",
    srcs = ["pngtest.c"],
    args = ["$(location :pngtest.png)"],
    data = ["pngtest.png"],
    visibility = ["//visibility:public"],
    deps = [
        ":libpng",
        "@zlib",
    ],
)
