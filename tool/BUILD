load("@rules_cc//cc:defs.bzl", "cc_binary")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_binary(
    name = "embed",
    srcs = ["embed.cpp"],
    copts = HASTUR_COPTS,
    target_compatible_with = select({
        # wasi doesn't support std::filesystem, but there's no reason to run
        # this binary as wasm anyway.
        "@platforms//os:wasi": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    visibility = ["//visibility:public"],
    deps = ["//util:arg_parser"],
)
