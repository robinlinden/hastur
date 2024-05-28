load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "zstd",
    srcs = glob([
        "lib/common/*.c",
        "lib/common/*.h",
        "lib/compress/*.c",
        "lib/compress/*.h",
        "lib/decompress/*.c",
        "lib/decompress/*.h",
        "lib/dictBuilder/*.c",
        "lib/dictBuilder/*.h",
    ]) + select({
        "@platforms//os:windows": [],
        "//conditions:default": glob(["lib/decompress/*.S"]),
    }),
    hdrs = [
        "lib/zdict.h",
        "lib/zstd.h",
        "lib/zstd_errors.h",
    ],
    copts = select({
        "@platforms//os:windows": [],
        "//conditions:default": ["-pthread"],
    }),
    includes = ["lib"],
    linkopts = select({
        "@platforms//os:windows": [],
        "//conditions:default": ["-pthread"],
    }),
    linkstatic = True,
    local_defines = [
        "XXH_NAMESPACE=ZSTD_",
        "ZSTD_BUILD_SHARED=OFF",
        "ZSTD_BUILD_STATIC=ON",
    ] + select({
        "@platforms//os:wasi": [],
        "@platforms//os:windows": [
            "ZSTD_DISABLE_ASM",
            "ZSTD_MULTITHREAD",
        ],
        "//conditions:default": ["ZSTD_MULTITHREAD"],
    }),
    visibility = ["//visibility:public"],
)
