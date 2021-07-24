load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "imgui",
    srcs = glob(["*.cpp"]) + ["misc/cpp/imgui_stdlib.cpp"],
    hdrs = glob(["*.h"]) + ["misc/cpp/imgui_stdlib.h"],
    includes = [
        ".",
        "misc/cpp/",
    ],
    visibility = ["//visibility:public"],
)
