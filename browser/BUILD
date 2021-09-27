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
        "//html",
        "//layout",
        "//protocol",
        "//style",
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
        "//html",
        "//layout",
        "//protocol",
        "//render",
        "//style",
        "//uri",
        "@imgui",
        "@imgui-sfml",
        "@sfml//:graphics",
        "@spdlog",
    ],
)
