load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "style",
    srcs = [
        "style.cpp",
        "styled_node.cpp",
    ],
    hdrs = [
        "style.h",
        "styled_node.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//css",
        "//dom",
    ],
)

cc_test(
    name = "style_test",
    size = "small",
    srcs = ["style_test.cpp"],
    deps = [
        ":style",
        "//etest",
    ],
)

cc_test(
    name = "styled_node_test",
    size = "small",
    srcs = ["styled_node_test.cpp"],
    deps = [
        ":style",
        "//etest",
    ],
)
