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
        "//tui",
        "@spdlog",
    ],
)

cc_binary(
    name = "gui",
    srcs = ["gui.cpp"],
    deps = [
        "@imgui",
        "@imgui-sfml",
        "@sfml//:graphics",
    ],
)
