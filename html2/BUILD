load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "html2",
    srcs = glob(
        include = ["*.cpp"],
        exclude = ["*_test.cpp"],
    ),
    hdrs = glob(["*.h"]),
    visibility = ["//visibility:public"],
    deps = [
        "//dom2",
        "//util",
        "@spdlog",
    ],
)

cc_test(
    name = "tokenizer_test",
    size = "small",
    srcs = ["tokenizer_test.cpp"],
    data = ["test/simple_page.html"],
    deps = [
        ":html2",
        "//etest",
    ],
)
