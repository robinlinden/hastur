load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "vulkan",
    hdrs = glob([
        "include/vulkan/*.h",
        "include/vk_video/*.h",
    ]),
    strip_include_prefix = "include",
    target_compatible_with = select({
        "@platforms//os:wasi": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    visibility = ["//visibility:public"],
)
