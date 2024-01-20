load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "imgui-sfml",
    srcs = ["imgui-SFML.cpp"],
    hdrs = glob(["*.h"]),
    defines = ["IMGUI_SFML_SHARED_LIB=0"],
    includes = ["."],
    linkopts = select({
        "@platforms//os:linux": ["-lGL"],
        "@platforms//os:macos": ["-lGL"],
        "@platforms//os:windows": ["-DEFAULTLIB:opengl32"],
    }),
    visibility = ["//visibility:public"],
    deps = [
        "@imgui",
        "@sfml//:graphics",
        "@sfml//:system",
    ] + select({
        "@platforms//os:macos": ["@sfml//:window_macos"],
        "//conditions:default": ["@sfml//:window"],
    }),
)
