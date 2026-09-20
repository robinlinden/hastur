load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

PTHREAD_OPTS = select({
    "@platforms//os:linux": ["-pthread"],
    "@platforms//os:macos": ["-pthread"],
    "//conditions:default": [],
})

cc_library(
    name = "engine",
    srcs = ["engine.cpp"],
    hdrs = ["engine.h"],
    copts = HASTUR_COPTS + PTHREAD_OPTS,
    implementation_deps = [
        "//archive:brotli",
        "//archive:zlib",
        "//archive:zstd",
        "//html",
        "//js",
        "//util:string",
        "@spdlog",
    ],
    linkopts = PTHREAD_OPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//css",
        "//dom",
        "//layout",
        "//protocol",
        "//style",
        "//type",
        "//type:naive",
        "//uri",
    ],
)

cc_test(
    name = "engine_test",
    size = "small",
    srcs = ["engine_test.cpp"],
    copts = HASTUR_COPTS,
    deps = [
        ":engine",
        "//css",
        "//dom",
        "//etest",
        "//gfx",
        "//protocol",
        "//style",
        "//type",
        "//type:naive",
        "//uri",
    ],
)
