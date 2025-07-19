load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

NET_COPTS = HASTUR_COPTS + select({
    "@platforms//os:linux": [
        # asio leaks this into our code.
        "-Wno-null-dereference",
        "-Wno-shadow",
        "-Wno-unknown-pragmas",
    ],
    "@platforms//os:macos": [
        "-Wno-null-dereference",
        "-Wno-shadow",
        "-Wno-unknown-pragmas",
    ],
    "//conditions:default": [],
})

cc_library(
    name = "net",
    srcs = glob(
        include = ["*.cpp"],
        exclude = ["*_test.cpp"],
    ),
    hdrs = glob(["*.h"]),
    copts = NET_COPTS,
    implementation_deps = [
        "@asio",
        "@boringssl//:ssl",
    ],
    target_compatible_with = select({
        # boringssl doesn't work on wasi but doesn't mark itself as incompatible.
        "@platforms//os:wasi": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    visibility = ["//visibility:public"],
)

cc_library(
    name = "test",
    testonly = True,
    hdrs = glob(["test/*.h"]),
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
)

[cc_test(
    name = src.removesuffix(".cpp"),
    size = "small",
    srcs = [src],
    copts = NET_COPTS,
    deps = [
        ":net",
        "//etest",
        "@asio",
    ],
) for src in glob(["*_test.cpp"])]
