load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

# TODO(robinlinden): This will live somewhere else once it's a bit more fleshed out.
cc_library(
    name = "about_handler",
    hdrs = ["about_handler.h"],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//protocol",
        "//uri",
        "@expected",
    ],
)

cc_test(
    name = "about_handler_test",
    srcs = ["about_handler_test.cpp"],
    copts = HASTUR_COPTS,
    deps = [
        ":about_handler",
        "//etest",
        "//protocol",
        "//uri",
        "@expected",
    ],
)

cc_library(
    name = "app",
    srcs = ["app.cpp"],
    hdrs = ["app.h"],
    copts = HASTUR_COPTS,
    tags = ["no-cross"],
    visibility = ["//visibility:public"],
    deps = [
        ":about_handler",
        "//css",
        "//dom",
        "//engine",
        "//geom",
        "//gfx",
        "//gfx:opengl",
        "//gfx:sfml",
        "//img:jpeg_turbo",
        "//img:png",
        "//layout",
        "//os:system_info",
        "//protocol",
        "//render",
        "//type",
        "//type:sfml",
        "//uri",
        "//url:percent_encode",
        "//util:history",
        "//util:string",
        "@expected",
        "@imgui",
        "@imgui-sfml",
        "@sfml//:graphics",
        "@sfml//:system",
        "@sfml//:window",
        "@spdlog",
    ],
)

cc_binary(
    name = "gui",
    srcs = ["main.cpp"],
    copts = HASTUR_COPTS,
    tags = ["no-cross"],
    visibility = ["//visibility:public"],
    deps = [
        ":app",
        "//os:system_info",
        "//util:arg_parser",
        "@spdlog",
    ],
)
