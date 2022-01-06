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
        "//util:string",
        "//util:unicode",
        "@spdlog",
    ],
)

data_files = {
    "tokenizer": ["test/simple_page.html"],
}

[cc_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    data = data_files.get(
        src[:-9],
        [],
    ),
    deps = [
        ":html2",
        "//etest",
    ],
) for src in glob(["*_test.cpp"])]
