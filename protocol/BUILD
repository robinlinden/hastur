load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

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
        "@fmt",
    ],
)

# file_handler_test needs some special treatment right now due to e.g.
# incompatibility w/ clang-cl.
[cc_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    deps = [
        ":protocol",
        "//etest",
    ],
) for src in glob(
    include = ["*_test.cpp"],
    exclude = ["file_handler_test.cpp"],
)]

# TODO(robinlinden): Get file_handler_test working with clang-cl. Right now it
# crashes due to a stack overflow when matching the regex in uri-parsing. This
# will go away once the spec-compliant url-parser is ready.
cc_test(
    name = "file_handler_test",
    size = "small",
    srcs = ["file_handler_test.cpp"],
    copts = HASTUR_COPTS,
    target_compatible_with = select({
        "//bzl:is_clang-cl": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    deps = [
        ":protocol",
        "//etest",
        "//uri",
        "@fmt",
    ],
)
