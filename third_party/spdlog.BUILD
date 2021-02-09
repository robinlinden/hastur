load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "spdlog",
    hdrs = glob(["include/**/*.h"]),
    linkopts = select({
        "@platforms//os:linux": ["-lpthread"],
        "@platforms//os:windows": [],
    }),
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
    deps = ["@fmt"],
)
