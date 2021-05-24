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
        "@spdlog",
    ],
)

cc_binary(
    name = "gui",
    srcs = ["gui.cpp"],
    deps = [
        "//dom",
        "//html",
        "//http",
        "@imgui",
        "@imgui-sfml",
        "@sfml//:graphics",
        "@spdlog",
    ],
)
