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
        "//util:string",
        "//util:unicode",
        "@fmt",
    ],
)

# TODO(robinlinden): Remove.
extra_deps = {
    "parser_states": [
        "//dom",
        "//html",
    ],
}

[cc_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    deps = [
        ":html2",
        "//etest",
        "@fmt",
    ] + extra_deps.get(
        src[:-9],
        [],
    ),
) for src in glob(
    include = ["*_test.cpp"],
    exclude = [
        "html5lib_test.cpp",
        "*_fuzz_test.cpp",
    ],
)]

cc_test(
    name = "html5lib_test",
    size = "small",
    srcs = ["html5lib_test.cpp"],
    copts = HASTUR_COPTS + select({
        # simdjson leaks a bunch of warnings into our code.
        "@platforms//os:windows": [
            "/wd4100",
            "/wd4706",
        ],
        "//conditions:default": [],
    }),
    data = ["@html5lib-tests//:tokenizer/test1.test"],
    # simdjson seems to blow up qemu when we run our aarch64 crosscompiled
    # tests.
    tags = ["no-cross"],
    deps = [
        ":html2",
        "//etest",
        "@simdjson",
    ],
)

[cc_fuzz_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    target_compatible_with = HASTUR_FUZZ_PLATFORMS,
    deps = [":html2"],
) for src in glob(["*_fuzz_test.cpp"])]
