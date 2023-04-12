load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

# Disable all extras because that's easier. We'll enable them later if we need them.
genrule(
    name = "config_header",
    srcs = ["include/unifex/config.hpp.in"],
    outs = ["include/unifex/config.hpp"],
    cmd = "cat $< | sed -r 's/^#cmakedefine01 (.*)/#define \\1 1/g' - | sed '/^#cmakedefine/d' | sed s/@libunifex_VERSION_MAJOR@/0/g | sed s/@libunifex_VERSION_MINOR@/1/g >$@",
)

cc_library(
    name = "unifex",
    srcs = glob(["source/*.cpp"]) + select({
        "@platforms//os:linux": glob(["source/linux/*"]),
        "@platforms//os:windows": glob([
            "include/unifex/win32/detail/*.hpp",
            "source/win32/*",
        ]),
    }),
    hdrs = glob([
        "include/unifex/*.hpp",
        "include/unifex/detail/*.hpp",
    ]) + [":config_header"] + select({
        "@platforms//os:linux": glob(["include/unifex/linux/*.hpp"]),
        "@platforms//os:windows": glob(["include/unifex/win32/*.hpp"]),
    }),
    includes = ["include/"],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)

# Not really tests, but lets us test that things are at least sort of working
# without setting up GoogleTest.
[cc_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    deps = [":unifex"],
) for src in glob(["examples/*.cpp"])]

test_suite(
    name = "unifex_tests",
)
