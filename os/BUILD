load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_library(
    name = "windows_setup",
    hdrs = ["windows_setup.h"],
    target_compatible_with = ["@platforms//os:windows"],
)

OS_DEPS = select({
    "@platforms//os:linux": [],
    "@platforms//os:macos": [],
    "@platforms//os:windows": [":windows_setup"],
})

OS_LOCAL_DEFINES = select({
    "@platforms//os:linux": [],
    "@platforms//os:macos": [],
    "@platforms//os:windows": ["WIN32_LEAN_AND_MEAN"],
})

cc_library(
    name = "memory",
    srcs = select({
        "@platforms//os:linux": ["memory_linux.cpp"],
        "@platforms//os:macos": ["memory_linux.cpp"],
        "@platforms//os:windows": ["memory_windows.cpp"],
    }),
    hdrs = ["memory.h"],
    copts = HASTUR_COPTS,
    linkopts = select({
        "@platforms//os:linux": [],
        "@platforms//os:macos": [],
        "@platforms//os:windows": [
            "-DEFAULTLIB:Kernel32",
        ],
    }),
    local_defines = OS_LOCAL_DEFINES,
    visibility = ["//visibility:public"],
    deps = OS_DEPS,
)

cc_test(
    name = "memory_test",
    size = "small",
    srcs = ["memory_test.cpp"],
    copts = HASTUR_COPTS,
    deps = [
        ":memory",
        "//etest",
    ],
)

cc_library(
    name = "system_info",
    srcs = select({
        "@platforms//os:linux": ["system_info_linux.cpp"],
        "@platforms//os:macos": ["system_info_linux.cpp"],
        "@platforms//os:windows": ["system_info_windows.cpp"],
    }),
    hdrs = ["system_info.h"],
    copts = HASTUR_COPTS,
    linkopts = select({
        "@platforms//os:linux": [],
        "@platforms//os:macos": [],
        "@platforms//os:windows": [
            "-DEFAULTLIB:Shcore",
            "-DEFAULTLIB:User32",
        ],
    }),
    local_defines = OS_LOCAL_DEFINES,
    visibility = ["//visibility:public"],
    deps = OS_DEPS,
)

cc_test(
    name = "system_info_linux_test",
    size = "small",
    srcs = ["system_info_linux_test.cpp"],
    copts = HASTUR_COPTS,
    target_compatible_with = ["@platforms//os:linux"],
    deps = [
        ":system_info",
        "//etest",
    ],
)

cc_library(
    name = "xdg",
    srcs = select({
        "@platforms//os:linux": ["xdg_linux.cpp"],
        "@platforms//os:macos": ["xdg_macos.cpp"],
        "@platforms//os:windows": ["xdg_windows.cpp"],
    }),
    hdrs = ["xdg.h"],
    copts = HASTUR_COPTS,
    linkopts = select({
        "@platforms//os:linux": [],
        "@platforms//os:macos": [],
        "@platforms//os:windows": [
            "-DEFAULTLIB:Shell32",
            "-DEFAULTLIB:Kernel32",
            "-DEFAULTLIB:Ole32",
        ],
    }),
    local_defines = OS_LOCAL_DEFINES,
    visibility = ["//visibility:public"],
    deps = OS_DEPS,
)

cc_test(
    name = "xdg_test",
    size = "small",
    srcs = ["xdg_test.cpp"],
    copts = HASTUR_COPTS,
    deps = [
        ":xdg",
        "//etest",
    ],
)

cc_test(
    name = "xdg_linux_test",
    size = "small",
    srcs = ["xdg_linux_test.cpp"],
    copts = HASTUR_COPTS,
    target_compatible_with = ["@platforms//os:linux"],
    deps = [
        ":xdg",
        "//etest",
    ],
)
