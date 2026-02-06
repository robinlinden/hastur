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
    implementation_deps = ["//net"],
    visibility = ["//visibility:public"],
    deps = [
        "//uri",
        "//util:string",
        "@expected",
    ],
)

[cc_fuzz_test(
    name = src.removesuffix(".cpp"),
    size = "small",
    testonly = True,
    srcs = [src],
    copts = HASTUR_COPTS,
    tags = ["no-cross"],
    target_compatible_with = HASTUR_FUZZ_PLATFORMS,
    deps = [
        ":protocol",
        "//net:test",
    ],
) for src in glob(["*_fuzz_test.cpp"])]

[cc_test(
    name = src.removesuffix(".cpp"),
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    deps = [
        ":protocol",
        "//etest",
        "//net:test",
        "//uri",
        "@expected",
    ],
) for src in glob(
    include = ["*_test.cpp"],
    exclude = ["*_fuzz_test.cpp"],
)]
