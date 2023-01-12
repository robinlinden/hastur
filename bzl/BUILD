filegroup(
    name = "xfail_test_runner",
    srcs = ["run_xfail_test"],
    visibility = ["//visibility:public"],
)

config_setting(
    name = "is_clang",
    flag_values = {"@bazel_tools//tools/cpp:compiler": "clang"},
)

config_setting(
    name = "is_clang-cl",
    flag_values = {"@bazel_tools//tools/cpp:compiler": "clang-cl"},
)

config_setting(
    name = "is_msvc",
    flag_values = {"@bazel_tools//tools/cpp:compiler": "msvc-cl"},
)
