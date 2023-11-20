load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_library(
    name = "type",
    hdrs = ["type.h"],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
)

cc_library(
    name = "naive",
    hdrs = ["naive.h"],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = [":type"],
)

cc_test(
    name = "naive_test",
    size = "small",
    srcs = ["naive_test.cpp"],
    copts = HASTUR_COPTS,
    deps = [
        ":naive",
        "//etest",
    ],
)

SFML_TYPE_COPTS = HASTUR_COPTS + select({
    # SFML leaks this into our code.
    "@platforms//os:linux": ["-Wno-implicit-fallthrough"],
    "//conditions:default": [],
})

cc_library(
    name = "sfml",
    srcs = ["sfml.cpp"],
    hdrs = ["sfml.h"],
    copts = SFML_TYPE_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":type",
        "//os:xdg",
        "//util:string",
        "@sfml//:graphics",
    ],
)
