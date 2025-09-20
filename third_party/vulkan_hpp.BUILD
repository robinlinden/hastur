load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_library(
    name = "hpp",
    hdrs = glob(["vulkan/*.hpp"]),
    includes = ["."],
    visibility = ["//visibility:public"],
    deps = ["@vulkan"],
)

cc_binary(
    name = "dispatch_loader_dynamic_test",
    srcs = ["tests/DispatchLoaderDynamic/DispatchLoaderDynamic.cpp"],
    defines = ["VULKAN_HPP_DISPATCH_LOADER_DYNAMIC"],
    visibility = ["//visibility:public"],
    deps = [":hpp"],
)
