load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "os",
    srcs = select({
        "@platforms//os:linux": ["linux.cpp"],
        "@platforms//os:windows": ["windows.cpp"],
    }),
    hdrs = ["os.h"],
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
    visibility = ["//visibility:public"],
)

cc_test(
    name = "os_test",
    size = "small",
    srcs = ["os_test.cpp"],
    deps = [
        ":os",
        "//etest",
    ],
)

cc_test(
    name = "linux_test",
    size = "small",
    srcs = ["linux_test.cpp"],
    target_compatible_with = ["@platforms//os:linux"],
    deps = [
        ":os",
        "//etest",
    ],
)
