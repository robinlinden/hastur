load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "html2",
    srcs = ["tokenizer.cpp"],
    hdrs = ["tokenizer.h"],
    visibility = ["//visibility:public"],
    deps = [
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
