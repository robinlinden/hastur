load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_library(
    name = "app",
    srcs = ["app.cpp"],
    hdrs = ["app.h"],
    copts = HASTUR_COPTS,
    tags = ["no-cross"],
    visibility = ["//visibility:public"],
    deps = [
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
