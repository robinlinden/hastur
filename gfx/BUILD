load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "gfx",
    srcs = ["gfx.cpp"],
    hdrs = ["gfx.h"],
    linkopts = select({
        "@platforms//os:linux": ["-lGL"],
        "@platforms//os:windows": ["-DEFAULTLIB:opengl32"],
    }),
    visibility = ["//visibility:public"],
    deps = ["//geom"],
)

cc_test(
    name = "gfx_test",
    size = "small",
    srcs = ["gfx_test.cpp"],
    deps = [
        ":gfx",
        "//etest",
    ],
)
