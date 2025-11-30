load("@rules_cc//cc:defs.bzl", "cc_binary")
load("//bzl:copts.bzl", "HASTUR_COPTS")

# spdlog/sinks/dist_sink.h:25:5: error: instantiation of function
# 'spdlog::sinks::base_sink<std::mutex>::base_sink' required here, but no
# definition is available [-Werror,-Wundefined-func-template]
#    25 |     dist_sink() = default;
#       |     ^
LINUX_COPTS = [
    "-Wno-undefined-func-template",
]

TUI_COPTS = HASTUR_COPTS + select({
    "@platforms//os:linux": LINUX_COPTS,
    "@platforms//os:macos": LINUX_COPTS,
    "//conditions:default": [],
})

cc_binary(
    name = "tui",
    srcs = ["tui.cpp"],
    copts = TUI_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//dom",
        "//engine",
        "//protocol",
        "//tui",
        "//uri",
        "@spdlog",
    ],
)
