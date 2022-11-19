load("@rules_cc//cc:defs.bzl", "cc_library")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_library(
    name = "tui",
    srcs = ["tui.cpp"],
    hdrs = ["tui.h"],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//layout",
        "@ftxui",
        "@spdlog",
    ],
)
