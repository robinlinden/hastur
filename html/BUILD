load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("@rules_fuzzing//fuzzing:cc_defs.bzl", "cc_fuzz_test")
load("//bzl:copts.bzl", "HASTUR_COPTS", "HASTUR_FUZZ_PLATFORMS")

cc_library(
    name = "html",
    srcs = glob(
        include = ["*.cpp"],
        exclude = ["*_test.cpp"],
    ),
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

[cc_test(
    name = "html5lib_tree_construction_test_%s" % test,
    size = "small",
    testonly = True,
    srcs = ["html5lib_tree_construction_test.cpp"],
    args = ["$(location @html5lib-tests//:tree-construction/%s.dat)" % test],
    copts = HASTUR_COPTS,
    data = ["@html5lib-tests//:tree-construction/%s.dat" % test],
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
) for test in [
    # TODO(robinlinden): Run more of the tree-construction tests.
    # "adoption01",
    # "adoption02",
    # "blocks",
    "comments01",
    "doctype01",
    # "domjs-unsafe",
    "entities01",
    "entities02",
    # "foreign-fragment",
    # "html5test-com",
    "inbody01",
    "isindex",
    # "main-element",
    # "math",
    # "menuitem-element",
    # "namespace-sensitivity",
    "noscript01",
    # "pending-spec-changes",
    # "pending-spec-changes-plain-text-unsafe",
    # "plain-text-unsafe",
    "quirks01",
    # "ruby",
    "scriptdata01",
    # "search-element",
    # "svg",
    # "tables01",
    # "template",
    # "tests1",
    # "tests2",
    # "tests3",
    # "tests4",
    # "tests5",
    # "tests6",
    # "tests7",
    # "tests8",
    # "tests9",
    # "tests10",
    # "tests11",
    # "tests12",
    # There is no tests13.
    "tests14",
    # "tests15",
    # "tests16",
    # "tests17",
    # "tests18",
    # "tests19",
    # "tests20",
    # "tests21",
    # "tests22",
    # "tests23",
    "tests24",
    # "tests25",
    # "tests26",
    # "tests_innerHTML_1",
    # "tricky01",
    # "webkit01",
    # "webkit02",
]]

[cc_fuzz_test(
    name = src.removesuffix(".cpp"),
    size = "small",
    testonly = True,
    srcs = [src],
    copts = HASTUR_COPTS,
    target_compatible_with = HASTUR_FUZZ_PLATFORMS,
    deps = [":html"],
) for src in glob(["*_fuzz_test.cpp"])]
