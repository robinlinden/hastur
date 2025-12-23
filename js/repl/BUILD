load("@rules_cc//cc:defs.bzl", "cc_binary")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_binary(
    name = "repl",
    srcs = ["main.cpp"],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//js",
        "//util:arg_parser",
    ],
)
