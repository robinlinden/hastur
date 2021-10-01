load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "render",
    srcs = ["render.cpp"],
    hdrs = ["render.h"],
    linkopts = select({
        "@platforms//os:linux": ["-lGL"],
        "@platforms//os:windows": ["-DEFAULTLIB:opengl32"],
    }),
    visibility = ["//visibility:public"],
    deps = [
        "//gfx",
        "//layout",
    ],
)
