load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("@rules_fuzzing//fuzzing:cc_defs.bzl", "cc_fuzz_test")
load("//bzl:copts.bzl", "HASTUR_COPTS", "HASTUR_FUZZ_PLATFORMS")

cc_library(
    name = "html",
    hdrs = glob(["*.h"]),
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//dom",
        "//html2",
    ],
)

[cc_test(
    name = src.removesuffix(".cpp"),
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    deps = [
        ":html",
        "//dom",
        "//etest",
        "//html2",
    ],
) for src in glob(
    include = ["*_test.cpp"],
    exclude = [
        "html5lib_*.cpp",
        "*_fuzz_test.cpp",
    ],
)]

# TODO(robinlinden): Run more of the tree-construction tests.
cc_test(
    name = "html5lib_tree_construction_test_runner",
    size = "small",
    testonly = True,
    srcs = ["html5lib_tree_construction_test.cpp"],
    args = ["$(location @html5lib-tests//:tree-construction/doctype01.dat)"],
    copts = HASTUR_COPTS,
    data = ["@html5lib-tests//:tree-construction/doctype01.dat"],
    target_compatible_with = select({
        # TODO(robinlinden): Investigate why we fail to open the test files when
        # running as a wasi binary.
        "@platforms//os:wasi": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    deps = [
        ":html",
        "//dom",
        "//etest",
    ],
)

[cc_fuzz_test(
    name = src.removesuffix(".cpp"),
    size = "small",
    testonly = True,
    srcs = [src],
    copts = HASTUR_COPTS,
    target_compatible_with = HASTUR_FUZZ_PLATFORMS,
    deps = [":html"],
) for src in glob(["*_fuzz_test.cpp"])]
