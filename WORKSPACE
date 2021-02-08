load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "asio",
    build_file = "//bazel:asio.BUILD",
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
    name = "ftxui",
    sha256 = "a56359a73a98bc05631d2952b286478ac89f1df3178fc63d52d7b1217ee5e527",
    build_file = "//bazel:ftxui.BUILD",
    strip_prefix = "FTXUI-d0eab413442d084d15a328c53fb132814daa58f3",
    urls = ["https://github.com/ArthurSonzogni/FTXUI/archive/d0eab413442d084d15a328c53fb132814daa58f3.tar.gz"],
)

http_archive(
    name = "platforms",
    sha256 = "460caee0fa583b908c622913334ec3c1b842572b9c23cf0d3da0c2543a1a157d",
    url = "https://github.com/bazelbuild/platforms/releases/download/0.0.3/platforms-0.0.3.tar.gz",
)
