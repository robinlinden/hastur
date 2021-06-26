load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "etest",
    srcs = ["etest.cpp"],
    hdrs = [
        "cxx_compat.h",
        "etest.h",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "etest_with_main",
    srcs = ["etest_main.cpp"],
    visibility = ["//visibility:public"],
    deps = [":etest"],
)

cc_test(
    name = "etest_test",
    size = "small",
    srcs = ["etest_test.cpp"],
    deps = [":etest_with_main"],
)

cc_test(
    name = "etest_expect_failure_test",
    size = "small",
    srcs = ["expect_failure_test.cpp"],
    deps = [":etest"],
)

cc_test(
    name = "etest_require_failure_test",
    size = "small",
    srcs = ["require_failure_test.cpp"],
    deps = [":etest"],
)
