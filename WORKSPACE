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
    sha256 = "6f640262999cd1fb33cf705922e453e835d2d20f3f06fe0d77f6426c19257308",
    strip_prefix = "boringssl-fc44652a42b396e1645d5e72aba053349992136a",
    url = "https://github.com/google/boringssl/archive/fc44652a42b396e1645d5e72aba053349992136a.tar.gz",
)

http_archive(
    name = "fmt",  # MIT
    build_file = "//third_party:fmt.BUILD",
    sha256 = "b06ca3130158c625848f3fb7418f235155a4d389b2abc3a6245fb01cb0eb1e01",
    strip_prefix = "fmt-8.0.1",
    url = "https://github.com/fmtlib/fmt/archive/8.0.1.tar.gz",
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
    sha256 = "6aaa8ec03c0dbbc371bc73afa1959e088846245e378256c631a0933469d2fee6",
    strip_prefix = "FTXUI-4d50dadb4167f356583cc2bb7b4257b340ba6275",
    url = "https://github.com/ArthurSonzogni/FTXUI/archive/4d50dadb4167f356583cc2bb7b4257b340ba6275.tar.gz",
)

http_archive(
    name = "imgui",  # MIT
    build_file = "//third_party:imgui.BUILD",
    sha256 = "35cb5ca0fb42cb77604d4f908553f6ef3346ceec4fcd0189675bdfb764f62b9b",
    strip_prefix = "imgui-1.84.2",
    url = "https://github.com/ocornut/imgui/archive/v1.84.2.tar.gz",
)

http_archive(
    name = "imgui-sfml",  # MIT
    build_file = "//third_party:imgui-sfml.BUILD",
    sha256 = "140fdcf916f78ad775a006534d027a4c048fb8770507861426aa75bce83783c6",
    strip_prefix = "imgui-sfml-8bc196c5eaadb342712407eb06fc2f53edfde227",
    url = "https://github.com/eliasdaler/imgui-sfml/archive/8bc196c5eaadb342712407eb06fc2f53edfde227.tar.gz",
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
    sha256 = "6fff9215f5cb81760be4cc16d033526d1080427d236e86d70bb02994f85e3d38",
    strip_prefix = "spdlog-1.9.2",
    url = "https://github.com/gabime/spdlog/archive/v1.9.2.tar.gz",
)

http_archive(
    name = "stb",  # MIT/Unlicense
    build_file = "//third_party:stb.BUILD",
    sha256 = "daeab82422dfdb1642278d74a24c3594b7a8ca782c53a6df0783d884a0f05c47",
    strip_prefix = "stb-c0c982601f40183e74d84a61237e968dca08380e",
    url = "https://github.com/nothings/stb/archive/c0c982601f40183e74d84a61237e968dca08380e.tar.gz",
)

http_archive(
    name = "udev-zero",  # ISC
    build_file = "//third_party:udev-zero.BUILD",
    sha256 = "68c20aefd7aa89abe446cf5ec76f5846315ded719f0665eabed9261cc3c7f47a",
    strip_prefix = "libudev-zero-1.0.0",
    url = "https://github.com/illiliti/libudev-zero/archive/1.0.0.tar.gz",
)

http_archive(
    name = "zlib",  # Zlib
    build_file = "//third_party:zlib.BUILD",
    sha256 = "629380c90a77b964d896ed37163f5c3a34f6e6d897311f1df2a7016355c45eff",
    strip_prefix = "zlib-1.2.11",
    url = "https://github.com/madler/zlib/archive/v1.2.11.tar.gz",
)
