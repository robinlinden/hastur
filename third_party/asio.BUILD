load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "asio",
    hdrs = glob([
        "include/**/*.hpp",
        "include/**/*.ipp",
    ]),
    defines = ["ASIO_HEADER_ONLY"],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)
