load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
load("//bzl:copts.bzl", "HASTUR_COPTS")

# spdlog/sinks/dist_sink.h:25:5: error: instantiation of function
# 'spdlog::sinks::base_sink<std::mutex>::base_sink' required here, but no
# definition is available [-Werror,-Wundefined-func-template]
#    25 |     dist_sink() = default;
#       |     ^
LINUX_COPTS = [
    "-Wno-undefined-func-template",
]

GUI_COPTS = HASTUR_COPTS + select({
    "@platforms//os:linux": LINUX_COPTS,
    "@platforms//os:macos": LINUX_COPTS,
    "//conditions:default": [],
})

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
    copts = GUI_COPTS,
    tags = ["no-cross"],
    visibility = ["//visibility:public"],
    deps = [
        ":app",
        "//os:system_info",
        "//util:arg_parser",
        "@spdlog",
    ],
)
