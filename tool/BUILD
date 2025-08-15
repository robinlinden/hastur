load("@rules_cc//cc:defs.bzl", "cc_binary")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_binary(
    name = "embed",
    srcs = ["embed.cpp"],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = ["//util:arg_parser"],
)
