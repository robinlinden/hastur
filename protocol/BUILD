load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("@rules_fuzzing//fuzzing:cc_defs.bzl", "cc_fuzz_test")
load("//bzl:copts.bzl", "HASTUR_COPTS", "HASTUR_FUZZ_PLATFORMS")

PTHREAD_OPTS = select({
    "@platforms//os:linux": ["-pthread"],
    "@platforms//os:macos": ["-pthread"],
    "//conditions:default": [],
})

cc_library(
    name = "protocol",
    srcs = glob(
        include = ["*.cpp"],
        exclude = ["*_test.cpp"],
    ),
    hdrs = glob(["*.h"]),
    copts = HASTUR_COPTS + PTHREAD_OPTS,
    implementation_deps = ["//net"],
    linkopts = PTHREAD_OPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//uri",
        "//util:string",
    ],
)

[cc_fuzz_test(
    name = src.removesuffix(".cpp"),
    size = "small",
    testonly = True,
    srcs = [src],
    copts = HASTUR_COPTS,
    tags = ["no-cross"],
    target_compatible_with = HASTUR_FUZZ_PLATFORMS,
    deps = [
        ":protocol",
        "//net:test",
    ],
) for src in glob(["*_fuzz_test.cpp"])]

[cc_test(
    name = src.removesuffix(".cpp"),
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS + PTHREAD_OPTS,
    linkopts = PTHREAD_OPTS,
    deps = [
        ":protocol",
        "//etest",
        "//net:test",
        "//uri",
    ],
) for src in glob(
    include = ["*_test.cpp"],
    exclude = ["*_fuzz_test.cpp"],
)]
