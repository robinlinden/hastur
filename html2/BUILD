load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("@rules_fuzzing//fuzzing:cc_defs.bzl", "cc_fuzz_test")
load("//bzl:copts.bzl", "HASTUR_COPTS", "HASTUR_FUZZ_PLATFORMS")

cc_library(
    name = "html2",
    srcs = glob(
        include = ["*.cpp"],
        exclude = ["*_test.cpp"],
    ),
    hdrs = glob(["*.h"]),
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//util:overloaded",
        "//util:string",
        "//util:unicode",
    ],
)

[cc_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    deps = [
        ":html2",
        "//etest",
        "@fmt",
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
    tags = ["manual"],
    target_compatible_with = HASTUR_FUZZ_PLATFORMS,
    deps = [":html2"],
) for src in glob(["*_fuzz_test.cpp"])]
