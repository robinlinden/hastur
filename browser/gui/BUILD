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
        "//gfx",
        "//gfx:opengl",
        "//gfx:sfml",
        "//layout",
        "//os:system_info",
        "//protocol",
        "//render",
        "//uri",
        "//util:history",
        "@fmt",
        "@imgui",
        "@imgui-sfml",
        "@sfml//:graphics",
        "@sfml//:system",
        "@sfml//:window",
        "@spdlog",
    ],
)
