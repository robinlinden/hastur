load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "protocol",
    srcs = [
        "get.cpp",
        "http.cpp",
    ],
    hdrs = [
        "get.h",
        "http.h",
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

cc_test(
    name = "protocol_test",
    size = "small",
    srcs = ["http_test.cpp"],
    deps = [
        ":protocol",
        "//etest",
    ],
)
