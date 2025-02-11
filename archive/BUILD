load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("@rules_fuzzing//fuzzing:cc_defs.bzl", "cc_fuzz_test")
load("//bzl:copts.bzl", "HASTUR_COPTS", "HASTUR_FUZZ_PLATFORMS")

cc_library(
    name = "zlib",
    srcs = ["zlib.cpp"],
    hdrs = ["zlib.h"],
    copts = HASTUR_COPTS,
    implementation_deps = ["@zlib"],
    visibility = ["//visibility:public"],
    deps = ["@expected"],
)

cc_library(
    name = "zstd",
    srcs = ["zstd.cpp"],
    hdrs = ["zstd.h"],
    copts = HASTUR_COPTS,
    implementation_deps = ["@zstd"],
    visibility = ["//visibility:public"],
    deps = ["@expected"],
)

cc_library(
    name = "brotli",
    srcs = ["brotli.cpp"],
    hdrs = ["brotli.h"],
    copts = HASTUR_COPTS,
    implementation_deps = [
        "@brotli//:brotli_inc",
        "@brotli//:brotlicommon",
        "@brotli//:brotlidec",
    ],
    visibility = ["//visibility:public"],
    deps = ["@expected"],
)

# TODO(robinlinden): Separate APIs for gzip and zlib.
alias(
    name = "gzip",
    actual = ":zlib",
)

[cc_test(
    name = src.removesuffix(".cpp"),
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    deps = [
        ":%s" % src.removesuffix("_test.cpp"),
        "//etest",
        "@expected",
    ],
) for src in glob(
    include = ["*_test.cpp"],
    exclude = ["*_fuzz_test.cpp"],
)]

[cc_fuzz_test(
    name = src.removesuffix(".cpp"),
    size = "small",
    testonly = True,
    srcs = [src],
    copts = HASTUR_COPTS,
    target_compatible_with = HASTUR_FUZZ_PLATFORMS,
    deps = [":%s" % src.removesuffix("_fuzz_test.cpp")],
) for src in glob(["*_fuzz_test.cpp"])]
