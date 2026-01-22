load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "imgui-sfml",
    srcs = ["imgui-SFML.cpp"],
    hdrs = glob(["*.h"]),
    defines = ["IMGUI_SFML_SHARED_LIB=0"],
    visibility = ["//visibility:public"],
    deps = [
        "@glad2",
        "@imgui",
        "@sfml//:graphics",
        "@sfml//:system",
        "@sfml//:window",
    ],
)
