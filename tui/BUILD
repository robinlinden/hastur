load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_library(
    name = "tui",
    srcs = ["tui.cpp"],
    hdrs = ["tui.h"],
    copts = HASTUR_COPTS,
    implementation_deps = [
        "//css",
        "//style",
        "@ftxui",
    ],
    visibility = ["//visibility:public"],
    deps = ["//layout"],
)

cc_test(
    name = "tui_test",
    size = "small",
    srcs = ["tui_test.cpp"],
    copts = HASTUR_COPTS,
    deps = [
        ":tui",
        "//css",
        "//dom",
        "//etest",
        "//layout",
        "//style",
        "//util:string",
    ],
)
