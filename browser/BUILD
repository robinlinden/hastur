load("@rules_cc//cc:defs.bzl", "cc_binary")

alias(
    name = "browser",
    actual = "gui",
)

cc_binary(
    name = "tui",
    srcs = ["tui.cpp"],
    linkopts = select({
        "@platforms//os:linux": ["-lpthread"],
        "@platforms//os:windows": [],
    }),
    deps = [
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
    deps = [
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
