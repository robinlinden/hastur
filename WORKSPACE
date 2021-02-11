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
    name = "catch2",
    sha256 = "e7eb70b3d0ac2ed7dcf14563ad808740c29e628edde99e973adad373a2b5e4df",
    strip_prefix = "Catch2-2.13.4",
    urls = ["https://github.com/catchorg/Catch2/archive/v2.13.4.tar.gz"],
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
    name = "spdlog",
    build_file = "//third_party:spdlog.BUILD",
    sha256 = "e20e6bd8f57e866eaf25a5417f0a38a116e537f1a77ac7b5409ca2b180cec0d5",
    strip_prefix = "spdlog-1.8.2",
    urls = ["https://github.com/gabime/spdlog/archive/v1.8.2.tar.gz"],
)
