load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "asio",
    srcs = glob(["asio/src/*.cpp"]),
    hdrs = glob([
        "asio/include/**/*.hpp",
        "asio/include/**/*.ipp",
    ]),
    defines = [
        "ASIO_NO_TYPEID",
        "ASIO_SEPARATE_COMPILATION",
    ],
    strip_include_prefix = "asio/include",
    visibility = ["//visibility:public"],
    deps = ["@boringssl//:ssl"],
)

cc_library(
    name = "header_only",
    hdrs = glob([
        "asio/include/**/*.hpp",
        "asio/include/**/*.ipp",
    ]),
    defines = ["ASIO_HEADER_ONLY"],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)
