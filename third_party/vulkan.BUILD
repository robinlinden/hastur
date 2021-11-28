load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "vulkan",
    hdrs = glob(["include/vulkan/*.h"]),
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "hpp",
    hdrs = glob(["include/vulkan/*.hpp"]),
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
    deps = [":vulkan"],
)
