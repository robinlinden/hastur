load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("@rules_fuzzing//fuzzing:cc_defs.bzl", "cc_fuzz_test")
load("//bzl:copts.bzl", "HASTUR_COPTS", "HASTUR_FUZZ_PLATFORMS")

cc_library(
    name = "protocol",
    srcs = glob(
        include = ["*.cpp"],
        exclude = ["*_test.cpp"],
    ),
    hdrs = glob(["*.h"]),
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//net",
        "//uri",
        "//util:string",
        "@expected",
        "@fmt",
    ],
)

[cc_fuzz_test(
    name = src[:-4],
    size = "small",
    testonly = True,
    srcs = [src],
    copts = HASTUR_COPTS,
    target_compatible_with = HASTUR_FUZZ_PLATFORMS,
    deps = [
        ":protocol",
        "//net:test",
    ],
) for src in glob(["*_fuzz_test.cpp"])]

[cc_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    deps = [
        ":protocol",
        "//etest",
        "//net:test",
        "//uri",
        "@expected",
        "@fmt",
    ],
) for src in glob(
    include = ["*_test.cpp"],
    exclude = ["*_fuzz_test.cpp"],
)]
