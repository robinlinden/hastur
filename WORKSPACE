load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# Bazel
# =========================================================

http_archive(
    name = "platforms",
    sha256 = "460caee0fa583b908c622913334ec3c1b842572b9c23cf0d3da0c2543a1a157d",
    url = "https://github.com/bazelbuild/platforms/releases/download/0.0.3/platforms-0.0.3.tar.gz",
)

# Third-party
# =========================================================

http_archive(
    name = "asio",
    build_file = "//third_party:asio.BUILD",
    sha256 = "4af9875df5497fdd507231f4b7346e17d96fc06fe10fd30e2b3750715a329113",
    strip_prefix = "asio-1.18.1",
    urls = ["https://downloads.sourceforge.net/project/asio/asio/1.18.1%20(Stable)/asio-1.18.1.tar.bz2"],
)

http_archive(
    name = "boringssl",
    sha256 = "af0e0b561629029332d832fd4a2f2fbed14206eb2f538037a746d82e5281ace8",
    strip_prefix = "boringssl-b92ed690b31bc2df9197dca73b38aaa1a9225a40",
    urls = ["https://github.com/google/boringssl/archive/b92ed690b31bc2df9197dca73b38aaa1a9225a40.tar.gz"],
)

http_archive(
    name = "fmt",
    build_file = "//third_party:fmt.BUILD",
    sha256 = "5cae7072042b3043e12d53d50ef404bbb76949dad1de368d7f993a15c8c05ecc",
    strip_prefix = "fmt-7.1.3",
    url = "https://github.com/fmtlib/fmt/archive/7.1.3.tar.gz",
)

http_archive(
    name = "ftxui",
    build_file = "//third_party:ftxui.BUILD",
    sha256 = "d38cb90331ff7dc43123dfe0a8565959459044378e60536d80ba3b0abc523ac7",
    strip_prefix = "FTXUI-73a3c24394621f31a59e6b1235dc5fd28f78d3d1",
    urls = ["https://github.com/ArthurSonzogni/FTXUI/archive/73a3c24394621f31a59e6b1235dc5fd28f78d3d1.tar.gz"],
)

http_archive(
    name = "imgui",
    build_file = "//third_party:imgui.BUILD",
    sha256 = "f7c619e03a06c0f25e8f47262dbc32d61fd033d2c91796812bf0f8c94fca78fb",
    strip_prefix = "imgui-1.81",
    url = "https://github.com/ocornut/imgui/archive/v1.81.tar.gz",
)

http_archive(
    name = "imgui-sfml",
    build_file = "//third_party:imgui-sfml.BUILD",
    sha256 = "843536a6c558579ab57f749c4c6e1e67e0f7b033ab434e0f9cf1ad38726ac51e",
    strip_prefix = "imgui-sfml-2.2",
    url = "https://github.com/eliasdaler/imgui-sfml/archive/v2.2.tar.gz",
)

http_archive(
    name = "sfml",
    build_file = "//third_party:sfml.BUILD",
    sha256 = "6124b5fe3d96e7f681f587e2d5b456cd0ec460393dfe46691f1933d6bde0640b",
    strip_prefix = "SFML-2.5.1",
    url = "https://github.com/SFML/SFML/archive/2.5.1.zip",
)

http_archive(
    name = "spdlog",
    build_file = "//third_party:spdlog.BUILD",
    sha256 = "e20e6bd8f57e866eaf25a5417f0a38a116e537f1a77ac7b5409ca2b180cec0d5",
    strip_prefix = "spdlog-1.8.2",
    urls = ["https://github.com/gabime/spdlog/archive/v1.8.2.tar.gz"],
)

http_archive(
    name = "stb",
    build_file = "//third_party:stb.BUILD",
    sha256 = "13a99ad430e930907f5611325ec384168a958bf7610e63e60e2fd8e7b7379610",
    strip_prefix = "stb-b42009b3b9d4ca35bc703f5310eedc74f584be58",
    url = "https://github.com/nothings/stb/archive/b42009b3b9d4ca35bc703f5310eedc74f584be58.tar.gz",
)
