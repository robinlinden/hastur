load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("@rules_fuzzing//fuzzing:cc_defs.bzl", "cc_fuzz_test")

cc_library(
    name = "uri",
    srcs = ["uri.cpp"],
    hdrs = ["uri.h"],
    visibility = ["//visibility:public"],
    deps = ["//util:string"],
)

cc_test(
    name = "uri_test",
    size = "small",
    srcs = ["uri_test.cpp"],
    deps = [
        ":uri",
        "//etest",
    ],
)

config_setting(
    name = "is_clang",
    flag_values = {"@bazel_tools//tools/cpp:compiler": "clang"},
)

cc_fuzz_test(
    name = "uri_fuzz_test",
    srcs = ["uri_fuzz_test.cpp"],
    target_compatible_with = select({
        ":is_clang": ["@platforms//os:linux"],
        "//conditions:default": ["@platforms//:incompatible"],
    }),
    deps = [":uri"],
)
