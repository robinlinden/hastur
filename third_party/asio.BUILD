load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "asio",
    srcs = glob(["src/*.cpp"]),
    hdrs = glob([
        "include/**/*.hpp",
        "include/**/*.ipp",
    ]),
    defines = [
        "ASIO_SEPARATE_COMPILATION",
        # TODO(robinlinden): Delete once https://github.com/chriskohlhoff/asio/pull/910 is merged.
        "ASIO_HAS_STD_INVOKE_RESULT",
    ],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
    deps = ["@boringssl//:ssl"],
)

cc_library(
    name = "header_only",
    hdrs = glob([
        "include/**/*.hpp",
        "include/**/*.ipp",
    ]),
    defines = ["ASIO_HEADER_ONLY"],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)
