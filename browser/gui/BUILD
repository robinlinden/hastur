load("@rules_cc//cc:defs.bzl", "cc_binary")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_binary(
    name = "gui",
    srcs = glob([
        "*.cpp",
        "*.h",
    ]),
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//css",
        "//dom",
        "//engine",
        "//geom",
        "//gfx",
        "//gfx:opengl",
        "//gfx:sfml",
        "//layout",
        "//os:system_info",
        "//protocol",
        "//render",
        "//type",
        "//type:sfml",
        "//uri",
        "//util:history",
        "@fmt",
        "@imgui",
        "@imgui-sfml",
        "@sfml//:graphics",
        "@sfml//:system",
        "@spdlog",
    ] + select({
        "@platforms//os:macos": ["@sfml//:window_macos"],
        "//conditions:default": ["@sfml//:window"],
    }),
)
