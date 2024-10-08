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
        "//unicode:util",
        "//util:string",
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
    name = "html5lib_test_runner",
    srcs = ["html5lib_test.cpp"],
    copts = HASTUR_COPTS + select({
        # simdjson leaks a bunch of warnings into our code.
        "@platforms//os:windows": [
            "/wd4100",
            "/wd4706",
        ],
        "//conditions:default": [],
    }),
    tags = [
        "manual",
        "no-cross",
    ],
    # simdjson seems to blow up qemu when we run our aarch64 crosscompiled
    # tests.
    deps = [
        ":html2",
        "//etest",
        "@simdjson",
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

# TODO(robinlinden): Look into the remaining tests.
[sh_test(
    name = "html5lib_{}".format(test.rpartition("/")[-1][:-5]),
    size = "small",
    srcs = [":test_with_arg"],
    args = [
        "$(location :html5lib_test_runner)",
        "$(location {})".format(test),
    ],
    data = [
        ":html5lib_test_runner",
        test,
    ],
    # simdjson seems to blow up qemu when we run our aarch64 crosscompiled
    # tests.
    tags = ["no-cross"],
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
    name = src[:-4],
    size = "small",
    testonly = True,
    srcs = [src],
    copts = HASTUR_COPTS,
    target_compatible_with = HASTUR_FUZZ_PLATFORMS,
    deps = [":html2"],
) for src in glob(["*_fuzz_test.cpp"])]
