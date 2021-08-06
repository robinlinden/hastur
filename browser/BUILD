load("@rules_cc//cc:defs.bzl", "cc_binary")

alias(
    name = "browser",
    actual = "tui",
)

cc_binary(
    name = "tui",
    srcs = ["tui.cpp"],
    linkopts = select({
        "@platforms//os:linux": ["-lpthread"],
        "@platforms//os:windows": [],
    }),
    deps = [
        "//html",
        "//http",
        "//layout",
        "//style",
        "//tui",
        "//uri",
        "@spdlog",
    ],
)

cc_binary(
    name = "gui",
    srcs = ["gui.cpp"],
    deps = [
        "//css",
        "//dom",
        "//html",
        "//http",
        "//layout",
        "//render",
        "//style",
        "//uri",
        "@imgui",
        "@imgui-sfml",
        "@sfml//:graphics",
        "@spdlog",
    ],
)
