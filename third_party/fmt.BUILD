load("@rules_cc//cc:defs.bzl", "cc_library")

# src/fmt.cc is used for C++20 modules.
cc_library(
    name = "fmt",
    srcs = glob(
        include = ["src/*.cc"],
        exclude = ["src/fmt.cc"],
    ),
    hdrs = glob(["include/**/*.h"]),
    defines = select({
        "@platforms//os:wasi": ["FMT_USE_FCNTL=0"],
        "//conditions:default": [],
    }),
    includes = ["include/"],
    local_defines = select({
        "@platforms//os:wasi": [
            # Not available in wasi, but that's fine because we never print to
            # files using fmt.
            "flockfile(x)",
            "funlockfile(x)",
        ],
        "//conditions:default": [],
    }),
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)
