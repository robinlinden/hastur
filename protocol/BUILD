load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "protocol",
    srcs = [
        "get.cpp",
        "http.cpp",
        "response.cpp",
    ],
    hdrs = [
        "get.h",
        "http.h",
        "iprotocol_handler.h",
        "multi_protocol_handler.h",
        "response.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//net",
        "//uri",
        "//util:string",
        "@fmt",
        "@range-v3",
    ],
)

[cc_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    deps = [
        ":protocol",
        "//etest",
    ],
) for src in glob(["*_test.cpp"])]
