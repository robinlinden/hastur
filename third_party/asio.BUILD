load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "asio",
    srcs = glob(["asio/src/*.cpp"]),
    hdrs = glob([
        "asio/include/**/*.hpp",
        "asio/include/**/*.ipp",
    ]),
    defines = [
        "ASIO_NO_DEPRECATED",
        "ASIO_NO_TYPEID",
        "ASIO_SEPARATE_COMPILATION",
    ],
    strip_include_prefix = "asio/include",
    visibility = ["//visibility:public"],
    deps = [
        "@boringssl//:crypto",
        "@boringssl//:ssl",
    ],
)
