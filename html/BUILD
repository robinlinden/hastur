load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")
load("@rules_fuzzing//fuzzing:cc_defs.bzl", "cc_fuzz_test")
load("@rules_shell//shell:sh_test.bzl", "sh_test")
load("//bzl:copts.bzl", "HASTUR_COPTS", "HASTUR_FUZZ_PLATFORMS")
load("//bzl:defs.bzl", "cc_xfail_test")

cc_library(
    name = "html",
    srcs = glob(
        include = ["*.cpp"],
        exclude = ["*_test.cpp"],
    ),
    hdrs = glob(["*.h"]),
    copts = HASTUR_COPTS,
    implementation_deps = [
        "//unicode:util",
        "//util:string",
    ],
    visibility = ["//visibility:public"],
    deps = ["//dom"],
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
    ],
) for src in glob(
    include = ["*_test.cpp"],
    exclude = [
        "html5lib_*.cpp",
        "*_fuzz_test.cpp",
    ],
)]

cc_binary(
    name = "html5lib_tree_construction_test_runner",
    testonly = True,
    srcs = ["html5lib_tree_construction_test.cpp"],
    copts = HASTUR_COPTS,
    deps = [
        ":html",
        "//dom",
        "//etest",
    ],
)

cc_binary(
    name = "html5lib_tokenizer_test_runner",
    testonly = True,
    srcs = ["html5lib_tokenizer_test.cpp"],
    copts = HASTUR_COPTS,
    deps = [
        ":html",
        "//etest",
        "//json",
    ],
)

genrule(
    name = "test_with_arg",
    testonly = True,
    outs = ["test_with_arg.sh"],
    cmd = """
        echo '#!/bin/sh' >$@
        echo 'exec "$$1" "$$2"' >>$@
        chmod +x $@
    """,
)

[sh_test(
    name = "html5lib_tree_construction_test_%s" % test,
    size = "small",
    srcs = [":test_with_arg"],
    args = [
        "$(location :html5lib_tree_construction_test_runner)",
        "$(location @html5lib-tests//:tree-construction/%s.dat)" % test,
    ],
    data = [
        ":html5lib_tree_construction_test_runner",
        "@html5lib-tests//:tree-construction/%s.dat" % test,
    ],
    target_compatible_with = select({
        # TODO(robinlinden): Investigate why we fail to open the test files when
        # running as a wasi binary.
        "@platforms//os:wasi": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
) for test in [
    "comments01",
    "doctype01",
    "entities01",
    "entities02",
    "inbody01",
    "isindex",
    "noscript01",
    "quirks01",
    "scriptdata01",
    "tests5",
    # There is no tests13.
    "tests14",
    "tests24",
    "tests25",
]]

# TODO(robinlinden): Stop xfailing these.
[cc_xfail_test(
    name = "html5lib_tree_construction_xfail_test_%s" % test,
    size = "small",
    srcs = ["html5lib_tree_construction_test.cpp"],
    args = ["$(location @html5lib-tests//:tree-construction/%s.dat)" % test],
    copts = HASTUR_COPTS,
    data = ["@html5lib-tests//:tree-construction/%s.dat" % test],
    target_compatible_with = select({
        "@platforms//os:wasi": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    deps = [
        ":html",
        "//dom",
        "//etest",
    ],
) for test in [
    "adoption01",
    "adoption02",
    "blocks",
    "domjs-unsafe",
    "foreign-fragment",
    "html5test-com",
    "main-element",
    "math",
    "menuitem-element",
    "namespace-sensitivity",
    "pending-spec-changes",
    "pending-spec-changes-plain-text-unsafe",
    "plain-text-unsafe",
    "ruby",
    "search-element",
    "svg",
    "tables01",
    "template",
    "tests1",
    "tests2",
    "tests3",
    "tests4",
    "tests6",
    "tests7",
    "tests8",
    "tests9",
    "tests10",
    "tests11",
    "tests12",
    "tests15",
    "tests16",
    "tests17",
    "tests18",
    "tests19",
    "tests20",
    "tests21",
    "tests22",
    "tests23",
    "tests26",
    "tests_innerHTML_1",
    "tricky01",
    "webkit01",
    "webkit02",
]]

# TODO(robinlinden): Look into the remaining tests.
[sh_test(
    name = "html5lib_tokenizer_{}".format(test.rpartition("/")[-1].removesuffix(".test")),
    size = "small",
    srcs = [":test_with_arg"],
    args = [
        "$(location :html5lib_tokenizer_test_runner)",
        "$(location {})".format(test),
    ],
    data = [
        ":html5lib_tokenizer_test_runner",
        test,
    ],
    target_compatible_with = select({
        # TODO(robinlinden): Investigate why we fail to open the test files when
        # running as a wasi binary.
        "@platforms//os:wasi": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
) for test in [
    "@html5lib-tests//:tokenizer/contentModelFlags.test",
    "@html5lib-tests//:tokenizer/domjs.test",
    "@html5lib-tests//:tokenizer/entities.test",
    "@html5lib-tests//:tokenizer/escapeFlag.test",
    "@html5lib-tests//:tokenizer/namedEntities.test",
    "@html5lib-tests//:tokenizer/numericEntities.test",
    "@html5lib-tests//:tokenizer/pendingSpecChanges.test",
    "@html5lib-tests//:tokenizer/test1.test",
    "@html5lib-tests//:tokenizer/test2.test",
    "@html5lib-tests//:tokenizer/test3.test",
    "@html5lib-tests//:tokenizer/test4.test",
    "@html5lib-tests//:tokenizer/unicodeChars.test",
    # "@html5lib-tests//:tokenizer/unicodeCharsProblematic.test",
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
