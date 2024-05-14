load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("@rules_fuzzing//fuzzing:cc_defs.bzl", "cc_fuzz_test")
load("//bzl:copts.bzl", "HASTUR_COPTS", "HASTUR_FUZZ_PLATFORMS")

cc_library(
    name = "zlib",
    srcs = ["zlib.cpp"],
    hdrs = ["zlib.h"],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = ["@zlib"],
)

cc_library(
    name = "zstd",
    srcs = ["zstd.cpp"],
    hdrs = ["zstd.h"],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = ["@zstd"],
)

# TODO(robinlinden): Separate APIs for gzip and zlib.
alias(
    name = "gzip",
    actual = ":zlib",
)

[cc_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    deps = [
        ":%s" % src[:-9],
        "//etest",
    ],
) for src in glob(
    include = ["*_test.cpp"],
    exclude = ["*_fuzz_test.cpp"],
)]

[cc_fuzz_test(
    name = src[:-4],
    size = "small",
    testonly = True,
    srcs = [src],
    copts = HASTUR_COPTS,
    target_compatible_with = HASTUR_FUZZ_PLATFORMS,
    deps = [":%s" % src[:-14]],
) for src in glob(["*_fuzz_test.cpp"])]
