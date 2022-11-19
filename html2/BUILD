load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("@rules_fuzzing//fuzzing:cc_defs.bzl", "cc_fuzz_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

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
        "//dom2",
        "//util:overloaded",
        "//util:string",
        "//util:unicode",
    ],
)

dependencies = {
    "tree_constructor": ["//dom2"],
}

[cc_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    deps = dependencies.get(
        src[:-9],
        [],
    ) + [
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
    target_compatible_with = ["@platforms//os:linux"],
    deps = [":html2"],
) for src in glob(["*_fuzz_test.cpp"])]
