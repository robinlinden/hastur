load("@rules_cc//cc:defs.bzl", "cc_binary")
load("//bzl:copts.bzl", "HASTUR_COPTS")

alias(
    name = "browser",
    actual = "gui",
)

[cc_library(
    name = hdr[:-2],
    hdrs = [hdr],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
) for hdr in glob(["*.h"])]

cc_binary(
    name = "tui",
    srcs = ["tui.cpp"],
    copts = HASTUR_COPTS,
    linkopts = select({
        "@platforms//os:linux": ["-lpthread"],
        "@platforms//os:windows": [],
    }),
    deps = [
        "//browser:branding",
        "//dom",
        "//engine",
        "//protocol",
        "//tui",
        "//uri",
        "@spdlog",
    ],
)

cc_binary(
    name = "gui",
    srcs = ["gui.cpp"] + glob(["gui/*"]),
    copts = HASTUR_COPTS,
    deps = [
        "//browser:branding",
        "//css",
        "//dom",
        "//engine",
        "//gfx",
        "//gfx:opengl",
        "//gfx:sfml",
        "//layout",
        "//os",
        "//protocol",
        "//render",
        "//uri",
        "//util:history",
        "@fmt",
        "@imgui",
        "@imgui-sfml",
        "@sfml//:graphics",
        "@sfml//:window",
        "@spdlog",
    ],
)
