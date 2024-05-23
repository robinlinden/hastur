load("@rules_cc//cc:defs.bzl", "cc_library")

# src/fmt.cc is used for C++20 modules.
cc_library(
    name = "fmt",
    srcs = glob(
        include = ["src/*.cc"],
        exclude = ["src/fmt.cc"],
    ),
    hdrs = glob(["include/**/*.h"]),
    includes = ["include/"],
    strip_include_prefix = "include",
    target_compatible_with = select({
        "@platforms//os:wasi": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    visibility = ["//visibility:public"],
)
