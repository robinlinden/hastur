load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "imgui-sfml",
    srcs = ["imgui-SFML.cpp"],
    hdrs = glob(["*.h"]),
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
