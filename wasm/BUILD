load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")
load("@rules_fuzzing//fuzzing:cc_defs.bzl", "cc_fuzz_test")
load("//bzl:copts.bzl", "HASTUR_COPTS", "HASTUR_FUZZ_PLATFORMS")

cc_library(
    name = "wasm",
    srcs = glob(
        include = ["*.cpp"],
        exclude = [
            "*_example.cpp",
            "*_test.cpp",
        ],
    ),
    hdrs = glob(["*.h"]),
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = ["@expected"],
)

[cc_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    deps = [
        ":wasm",
        "//etest",
    ],
) for src in glob(
    include = ["*_test.cpp"],
    exclude = ["*_fuzz_test.cpp"],
)]

[cc_fuzz_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    target_compatible_with = HASTUR_FUZZ_PLATFORMS,
    deps = [":wasm"],
) for src in glob(["*_fuzz_test.cpp"])]

cc_binary(
    name = "wasm_example",
    srcs = ["wasm_example.cpp"],
    copts = HASTUR_COPTS,
    deps = [":wasm"],
)
