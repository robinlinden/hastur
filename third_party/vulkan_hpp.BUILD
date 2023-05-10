load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "hpp",
    hdrs = glob(["vulkan/*.hpp"]),
    includes = ["."],
    visibility = ["//visibility:public"],
    deps = ["@vulkan"],
)
