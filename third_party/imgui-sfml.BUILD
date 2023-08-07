load("@rules_cc//cc:defs.bzl", "cc_library")

IMGUI_SFML_COPTS = select({
    "@platforms//os:linux": [
        "-Wno-switch",
    ],
    "//conditions:default": [],
})

cc_library(
    name = "imgui-sfml",
    srcs = ["imgui-SFML.cpp"],
    hdrs = glob(["*.h"]),
    copts = IMGUI_SFML_COPTS,
    defines = ["IMGUI_SFML_SHARED_LIB=0"],
    includes = ["."],
    linkopts = select({
        "@platforms//os:linux": ["-lGL"],
        "@platforms//os:windows": ["-DEFAULTLIB:opengl32"],
    }),
    visibility = ["//visibility:public"],
    deps = [
        "@imgui",
        "@sfml//:graphics",
        "@sfml//:system",
        "@sfml//:window",
    ],
)
