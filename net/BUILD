load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

# asio/ip/resolver_base.hpp:68:5: error: declaration shadows a variable in
# namespace 'asio::ip' [-Werror,-Wshadow]
#    68 |     v4_mapped = ASIO_OS_DEF(AI_V4MAPPED),
#       |     ^
#
# asio/execution/outstanding_work.hpp:358:32: error: out-of-line definition of
# constexpr static data member is redundant in C++17 and is deprecated
# [-Werror,-Wdeprecated-redundant-constexpr-static-def]
#   358 | const T outstanding_work_t<I>::static_query_v;
#       |                                ^
LINUX_COPTS = [
    "-Wno-deprecated",
    "-Wno-shadow",
]

NET_COPTS = HASTUR_COPTS + select({
    "@platforms//os:linux": LINUX_COPTS,
    "@platforms//os:macos": LINUX_COPTS,
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
