load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "vulkan",
    hdrs = glob(["include/**/*.h"]),
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
    linkopts = ["-lvulkan"],
)

cc_library(
    name = "hpp",
    hdrs = glob(["include/vulkan/*.hpp"]),
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
    deps = [":vulkan"],
)
