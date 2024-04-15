filegroup(
    name = "xfail_test_runner",
    srcs = ["run_xfail_test"],
    visibility = ["//visibility:public"],
)

alias(
    name = "linux_or_macos",
    actual = select({
        "@platforms//os:linux": "@platforms//os:linux",
        "@platforms//os:macos": "@platforms//os:macos",
        "//conditions:default": "@platforms//:incompatible",
    }),
    visibility = ["//visibility:public"],
)
