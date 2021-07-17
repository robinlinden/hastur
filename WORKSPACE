load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# Bazel
# =========================================================

http_archive(
    name = "platforms",  # Apache-2.0
    sha256 = "079945598e4b6cc075846f7fd6a9d0857c33a7afc0de868c2ccb96405225135d",
    url = "https://github.com/bazelbuild/platforms/releases/download/0.0.4/platforms-0.0.4.tar.gz",
)

# Third-party
# =========================================================

http_archive(
    name = "asio",  # BSL-1.0
    build_file = "//third_party:asio.BUILD",
    sha256 = "3ac05d4586d4b10afc28ff09017639652fb04feb9e575f9d48410db3ab27f9f2",
    strip_prefix = "asio-1.18.2",
    url = "https://downloads.sourceforge.net/project/asio/asio/1.18.2%20(Stable)/asio-1.18.2.tar.bz2",
)

http_archive(
    name = "boringssl",  # OpenSSL + ISC
    sha256 = "e06c3984d087297d0e5514b407d0385eca8b37a2f07ecfb60501be8786ad2500",
    strip_prefix = "boringssl-5ba39c1948c20827c027ebca6400127edbb313e7",
    url = "https://github.com/google/boringssl/archive/5ba39c1948c20827c027ebca6400127edbb313e7.tar.gz",
)

http_archive(
    name = "fmt",  # MIT
    build_file = "//third_party:fmt.BUILD",
    sha256 = "5cae7072042b3043e12d53d50ef404bbb76949dad1de368d7f993a15c8c05ecc",
    strip_prefix = "fmt-7.1.3",
    url = "https://github.com/fmtlib/fmt/archive/7.1.3.tar.gz",
)

http_archive(
    name = "freetype2",  # FTL
    build_file = "//third_party:freetype2.BUILD",
    sha256 = "33a28fabac471891d0523033e99c0005b95e5618dc8ffa7fa47f9dadcacb1c9b",
    strip_prefix = "freetype-2.8",
    urls = [
        "https://mirror.bazel.build/download.savannah.gnu.org/releases/freetype/freetype-2.8.tar.gz",
        "http://download.savannah.gnu.org/releases/freetype/freetype-2.8.tar.gz",
    ],
)

http_archive(
    name = "ftxui",  # MIT
    build_file = "//third_party:ftxui.BUILD",
    sha256 = "ec9d7688007bc7c8ef9cefd4ceec359d383c13d6b3efd978484a063183d79154",
    strip_prefix = "FTXUI-21d746e8586a59a39ed5c73317812f17264e68d5",
    url = "https://github.com/ArthurSonzogni/FTXUI/archive/21d746e8586a59a39ed5c73317812f17264e68d5.tar.gz",
)

http_archive(
    name = "imgui",  # MIT
    build_file = "//third_party:imgui.BUILD",
    sha256 = "ccf3e54b8d1fa30dd35682fc4f50f5d2fe340b8e29e08de71287d0452d8cc3ff",
    strip_prefix = "imgui-1.83",
    url = "https://github.com/ocornut/imgui/archive/v1.83.tar.gz",
)

http_archive(
    name = "imgui-sfml",  # MIT
    build_file = "//third_party:imgui-sfml.BUILD",
    sha256 = "4e2f520916d1d676a4553f5c266ed869e32808b0f4681b9b603280257323a45b",
    strip_prefix = "imgui-sfml-2.3",
    url = "https://github.com/eliasdaler/imgui-sfml/archive/v2.3.tar.gz",
)

http_archive(
    name = "libpng",  # Libpng
    build_file = "//third_party:libpng.BUILD",
    sha256 = "ca74a0dace179a8422187671aee97dd3892b53e168627145271cad5b5ac81307",
    strip_prefix = "libpng-1.6.37",
    url = "https://github.com/glennrp/libpng/archive/v1.6.37.tar.gz",
)

http_archive(
    name = "sfml",  # Zlib
    build_file = "//third_party:sfml.BUILD",
    sha256 = "6124b5fe3d96e7f681f587e2d5b456cd0ec460393dfe46691f1933d6bde0640b",
    strip_prefix = "SFML-2.5.1",
    url = "https://github.com/SFML/SFML/archive/2.5.1.zip",
)

http_archive(
    name = "spdlog",  # MIT
    build_file = "//third_party:spdlog.BUILD",
    sha256 = "944d0bd7c763ac721398dca2bb0f3b5ed16f67cef36810ede5061f35a543b4b8",
    strip_prefix = "spdlog-1.8.5",
    url = "https://github.com/gabime/spdlog/archive/v1.8.5.tar.gz",
)

http_archive(
    name = "stb",  # MIT/Unlicense
    build_file = "//third_party:stb.BUILD",
    sha256 = "7e1dfff854ca68ed324f6b1fcb55f8d365d41e23e931ef16057221f305a52b1d",
    strip_prefix = "stb-3a1174060a7dd4eb652d4e6854bc4cd98c159200",
    url = "https://github.com/nothings/stb/archive/3a1174060a7dd4eb652d4e6854bc4cd98c159200.tar.gz",
)

http_archive(
    name = "udev-zero",  # ISC
    build_file = "//third_party:udev-zero.BUILD",
    sha256 = "23f1046f13403ec217665193e78dad6cdb0af54e105a9aaf8440a846e66a1d62",
    strip_prefix = "libudev-zero-0.5.2",
    url = "https://github.com/illiliti/libudev-zero/archive/0.5.2.tar.gz",
)

http_archive(
    name = "zlib",  # Zlib
    build_file = "//third_party:zlib.BUILD",
    sha256 = "629380c90a77b964d896ed37163f5c3a34f6e6d897311f1df2a7016355c45eff",
    strip_prefix = "zlib-1.2.11",
    url = "https://github.com/madler/zlib/archive/v1.2.11.tar.gz",
)
