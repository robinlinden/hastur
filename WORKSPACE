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
    sha256 = "204374d3cadff1b57a63f4c343cbadcee28374c072dc04b549d772dbba9f650c",
    strip_prefix = "asio-1.20.0",
    url = "https://downloads.sourceforge.net/project/asio/asio/1.20.0%20(Stable)/asio-1.20.0.tar.bz2",
)

http_archive(
    name = "boringssl",  # OpenSSL + ISC
    sha256 = "57619c1ae003b560abe044d930bbb7ffe32a52d3abe32284da923865ab5a0dff",
    strip_prefix = "boringssl-c76e2b1fd16c20cf5908c33e34d392a276dde806",
    url = "https://github.com/google/boringssl/archive/c76e2b1fd16c20cf5908c33e34d392a276dde806.tar.gz",
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
    sha256 = "a45c6b403413abd5706f3582f04c8339d26397c4304b78fa552f2215df64101f",
    strip_prefix = "freetype-2.11.0",
    url = "https://download.savannah.gnu.org/releases/freetype/freetype-2.11.0.tar.gz",
)

http_archive(
    name = "ftxui",  # MIT
    build_file = "//third_party:ftxui.BUILD",
    sha256 = "371fc4224876411b90f2e80aae12c9f894765aaadc9bbffdcd09ac6b373ea93f",
    strip_prefix = "FTXUI-0.11",
    url = "https://github.com/ArthurSonzogni/FTXUI/archive/v0.11.tar.gz",
)

http_archive(
    name = "glew",  # BSD-3-Clause
    build_file = "//third_party:glew.BUILD",
    sha256 = "d4fc82893cfb00109578d0a1a2337fb8ca335b3ceccf97b97e5cc7f08e4353e1",
    strip_prefix = "glew-2.2.0",
    url = "https://github.com/nigels-com/glew/releases/download/glew-2.2.0/glew-2.2.0.tgz",
)

http_archive(
    name = "imgui",  # MIT
    build_file = "//third_party:imgui.BUILD",
    sha256 = "7ed49d1f4573004fa725a70642aaddd3e06bb57fcfe1c1a49ac6574a3e895a77",
    strip_prefix = "imgui-1.85",
    url = "https://github.com/ocornut/imgui/archive/v1.85.tar.gz",
)

http_archive(
    name = "imgui-sfml",  # MIT
    build_file = "//third_party:imgui-sfml.BUILD",
    sha256 = "94817346d5d0a96ab69d62451c19a8c24299264fcd9704cc130060bb051a8761",
    strip_prefix = "imgui-sfml-2.4",
    url = "https://github.com/eliasdaler/imgui-sfml/archive/v2.4.tar.gz",
)

http_archive(
    name = "libpng",  # Libpng
    build_file = "//third_party:libpng.BUILD",
    sha256 = "ca74a0dace179a8422187671aee97dd3892b53e168627145271cad5b5ac81307",
    strip_prefix = "libpng-1.6.37",
    url = "https://github.com/glennrp/libpng/archive/v1.6.37.tar.gz",
)

http_archive(
    name = "range-v3",  # BSL-1.0
    sha256 = "3575e4645cd1a7d42fa42a6b016e75a7c72d31d13f72ee4e5bb9773d36303258",
    strip_prefix = "range-v3-83783f578e0e6666d68a3bf17b0038a80e62530e",
    url = "https://github.com/ericniebler/range-v3/archive/83783f578e0e6666d68a3bf17b0038a80e62530e.tar.gz",
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
    sha256 = "936b4e506b5f55db178207e528ecdf5a411f67431447767d06c9b7061765cd7e",
    strip_prefix = "stb-af1a5bc352164740c1cc1354942b1c6b72eacb8a",
    url = "https://github.com/nothings/stb/archive/af1a5bc352164740c1cc1354942b1c6b72eacb8a.tar.gz",
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
