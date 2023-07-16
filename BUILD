load("@hedron_compile_commands//:refresh_compile_commands.bzl", "refresh_compile_commands")

refresh_compile_commands(
    name = "refresh_compile_commands",
    tags = ["manual"],
)

filegroup(
    name = "clang_tidy_config",
    srcs = [".clang-tidy"],
    visibility = ["//visibility:public"],
)

platform(
    name = "x64_windows-clang-cl",
    constraint_values = [
        "@bazel_tools//tools/cpp:clang-cl",
        "@platforms//cpu:x86_64",
        "@platforms//os:windows",
    ],
)
