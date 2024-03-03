load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "internal_hdrs",
    hdrs = glob(["src/**"]),
    strip_include_prefix = "src/",
)

cc_library(
    name = "simdjson",
    srcs = ["src/simdjson.cpp"],
    hdrs = glob(["include/**/*.h"]),
    defines = ["SIMDJSON_DISABLE_DEPRECATED_API=1"],
    implementation_deps = [":internal_hdrs"],
    includes = ["include/"],
    visibility = ["//visibility:public"],
)
