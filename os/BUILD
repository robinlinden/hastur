load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_library(
    name = "os",
    srcs = select({
        "@platforms//os:linux": ["linux.cpp"],
        "@platforms//os:windows": ["windows.cpp"],
    }),
    hdrs = ["os.h"],
    copts = HASTUR_COPTS,
    linkopts = select({
        "@platforms//os:linux": [],
        "@platforms//os:windows": [
            "-DEFAULTLIB:Kernel32",
            "-DEFAULTLIB:Ole32",
            "-DEFAULTLIB:Shcore",
            "-DEFAULTLIB:Shell32",
            "-DEFAULTLIB:User32",
        ],
    }),
    local_defines = select({
        "@platforms//os:linux": [],
        "@platforms//os:windows": ["WIN32_LEAN_AND_MEAN"],
    }),
    visibility = ["//visibility:public"],
)

cc_test(
    name = "os_test",
    size = "small",
    srcs = ["os_test.cpp"],
    copts = HASTUR_COPTS,
    deps = [
        ":os",
        "//etest",
    ],
)

cc_test(
    name = "linux_test",
    size = "small",
    srcs = ["linux_test.cpp"],
    copts = HASTUR_COPTS,
    target_compatible_with = ["@platforms//os:linux"],
    deps = [
        ":os",
        "//etest",
    ],
)
