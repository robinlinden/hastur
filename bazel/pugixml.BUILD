load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "pugixml",
    srcs = glob(["src/*.cpp"]),
    hdrs = glob(["src/*.hpp"]),
    strip_include_prefix = "src",
    visibility = ["//visibility:public"],
)
