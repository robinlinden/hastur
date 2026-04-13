load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

# The unicode data is separated into different headers as the tables generated
# can be quite large, and you might not want all of them if you're only using
# one little corner of the unicode bits.

cc_binary(
    name = "canonical_combining_class_data_processor",
    srcs = ["canonical_combining_class_data_processor.cpp"],
    copts = HASTUR_COPTS,
    target_compatible_with = select({
        # Contains filesystem I/O. Our current wasm/wasi-setup doesn't like that.
        "@platforms//os:wasi": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    deps = ["//util:string"],
)

genrule(
    name = "generate_canonical_combining_class_data",
    srcs = ["@ucd//:UnicodeData.txt"],
    outs = ["canonical_combining_class_data.h"],
    cmd = "$(location :canonical_combining_class_data_processor) $(location @ucd//:UnicodeData.txt) >$@",
    tools = [":canonical_combining_class_data_processor"],
)

cc_binary(
    name = "decomposition_data_processor",
    srcs = ["decomposition_data_processor.cpp"],
    copts = HASTUR_COPTS,
    target_compatible_with = select({
        # Contains filesystem I/O. Our current wasm/wasi-setup doesn't like that.
        "@platforms//os:wasi": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    deps = ["//util:string"],
)

genrule(
    name = "generate_decomposition_data",
    srcs = ["@ucd//:UnicodeData.txt"],
    outs = ["decomposition_data.h"],
    cmd = "$(location :decomposition_data_processor) $(location @ucd//:UnicodeData.txt) >$@",
    tools = [":decomposition_data_processor"],
)

cc_library(
    name = "normalization",
    srcs = [
        "normalization.cpp",
        ":generate_decomposition_data",
    ],
    hdrs = ["normalization.h"],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = [":util"],
)

cc_library(
    name = "util",
    hdrs = ["util.h"],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
)

[cc_test(
    name = src.removesuffix(".cpp"),
    size = "small",
    srcs = [
        src,
        ":generate_canonical_combining_class_data",
    ],
    copts = HASTUR_COPTS,
    deps = [
        ":normalization",
        ":util",
        "//etest",
    ],
) for src in glob(["*_test.cpp"])]
