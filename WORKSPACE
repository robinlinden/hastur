load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# Bazel
# =========================================================

http_archive(
    name = "platforms",  # Apache-2.0
    sha256 = "5308fc1d8865406a49427ba24a9ab53087f17f5266a7aabbfc28823f3916e1ca",
    url = "https://github.com/bazelbuild/platforms/releases/download/0.0.6/platforms-0.0.6.tar.gz",
)

http_archive(
    name = "rules_fuzzing",
    sha256 = "d9002dd3cd6437017f08593124fdd1b13b3473c7b929ceb0e60d317cb9346118",
    strip_prefix = "rules_fuzzing-0.3.2",
    url = "https://github.com/bazelbuild/rules_fuzzing/archive/v0.3.2.zip",
)

load("@rules_fuzzing//fuzzing:repositories.bzl", "rules_fuzzing_dependencies")

rules_fuzzing_dependencies()

load("@rules_fuzzing//fuzzing:init.bzl", "rules_fuzzing_init")

rules_fuzzing_init()

# Misc tools
# =========================================================

# HEAD as of 2022-12-17.
http_archive(
    name = "hedron_compile_commands",
    sha256 = "9b5683e6e0d764585f41639076f0be421a4c495c8f993c186e4449977ce03e5e",
    strip_prefix = "bazel-compile-commands-extractor-c6cd079bef5836293ca18e55aac6ef05134c3a9d",
    url = "https://github.com/hedronvision/bazel-compile-commands-extractor/archive/c6cd079bef5836293ca18e55aac6ef05134c3a9d.tar.gz",
)

load("@hedron_compile_commands//:workspace_setup.bzl", "hedron_compile_commands_setup")

hedron_compile_commands_setup()

# Third-party
# =========================================================

http_archive(
    name = "asio",  # BSL-1.0
    build_file = "//third_party:asio.BUILD",
    sha256 = "b31c63867daaba0e460ee2c85dc508a52c81db0a7318e0d2147f444b26f80ed7",
    strip_prefix = "asio-asio-1-27-0",
    url = "https://github.com/chriskohlhoff/asio/archive/asio-1-27-0.tar.gz",
)

# boringssl//:ssl cheats and pulls in private includes from boringssl//:crypto.
http_archive(
    name = "boringssl",  # OpenSSL + ISC
    patch_cmds = ["sed -i '33i package(features=[\"-layering_check\"])' BUILD"],
    sha256 = "cee1d0e73a01454da2b5195eb4cce19a1cfd4b54620e8f4c5bf15f6349aadb22",
    strip_prefix = "boringssl-b3e37ac6dc845f2ee3c2db928cb531825ce24ee5",
    url = "https://github.com/google/boringssl/archive/b3e37ac6dc845f2ee3c2db928cb531825ce24ee5.tar.gz",
)

http_archive(
    name = "ctre",  # Apache-2.0
    build_file = "//third_party:ctre.BUILD",
    sha256 = "d00d7eaa0e22f2fdaa947a532b81b6fc35880acf4887b50a5ac9bfb7411ced03",
    strip_prefix = "compile-time-regular-expressions-3.7.1",
    url = "https://github.com/hanickadot/compile-time-regular-expressions/archive/v3.7.1.tar.gz",
)

http_archive(
    name = "expected",  # CC0-1.0
    build_file = "//third_party:expected.BUILD",
    sha256 = "67b4216c80eae2959222a5da93bb58839ebef0ed37f3c402b930fa5b263244f9",
    strip_prefix = "expected-3e304a4bdd00feef8d41f5b388c37bd9d6108025",
    url = "https://github.com/tartanllama/expected/archive/3e304a4bdd00feef8d41f5b388c37bd9d6108025.tar.gz",
)

http_archive(
    name = "fmt",  # MIT
    build_file = "//third_party:fmt.BUILD",
    sha256 = "5dea48d1fcddc3ec571ce2058e13910a0d4a6bab4cc09a809d8b1dd1c88ae6f2",
    strip_prefix = "fmt-9.1.0",
    url = "https://github.com/fmtlib/fmt/archive/9.1.0.tar.gz",
)

http_archive(
    name = "freetype2",  # FTL
    build_file = "//third_party:freetype2.BUILD",
    sha256 = "0e72cae32751598d126cfd4bceda909f646b7231ab8c52e28abb686c20a2bea1",
    strip_prefix = "freetype-VER-2-12-1",
    url = "https://github.com/freetype/freetype/archive/VER-2-12-1.tar.gz",
)

http_archive(
    name = "ftxui",  # MIT
    build_file = "//third_party:ftxui.BUILD",
    sha256 = "7276e4117429ebf8e34ea371c3ea4e66eb99e0f234cb4c5c85fca17174a53dfa",
    strip_prefix = "FTXUI-4.0.0",
    url = "https://github.com/ArthurSonzogni/FTXUI/archive/v4.0.0.tar.gz",
)

http_archive(
    name = "glew",  # BSD-3-Clause
    build_file = "//third_party:glew.BUILD",
    sha256 = "d4fc82893cfb00109578d0a1a2337fb8ca335b3ceccf97b97e5cc7f08e4353e1",
    strip_prefix = "glew-2.2.0",
    url = "https://github.com/nigels-com/glew/releases/download/glew-2.2.0/glew-2.2.0.tgz",
)

http_archive(
    name = "icu",  # Unicode-DFS-2016
    build_file = "//third_party:icu.BUILD",
    patch_cmds = [
        "rm source/common/BUILD.bazel",
        "rm source/stubdata/BUILD.bazel",
    ],
    sha256 = "a2d2d38217092a7ed56635e34467f92f976b370e20182ad325edea6681a71d68",
    strip_prefix = "icu",
    url = "https://github.com/unicode-org/icu/releases/download/release-72-1/icu4c-72_1-src.tgz",
)

http_archive(
    name = "imgui",  # MIT
    build_file = "//third_party:imgui.BUILD",
    sha256 = "3b665fadd5580b7ef494d5d8bb1c12b2ec53ee723034caf43332956381f5d631",
    strip_prefix = "imgui-1.89.3",
    url = "https://github.com/ocornut/imgui/archive/v1.89.3.tar.gz",
)

http_archive(
    name = "imgui-sfml",  # MIT
    build_file = "//third_party:imgui-sfml.BUILD",
    sha256 = "c9f5f5ed92ad30afb64f32e2e0d4b4050c59de465f759330e972b90891798581",
    strip_prefix = "imgui-sfml-49dbecb43040449cccb3bfc43e3472cee94da417",
    url = "https://github.com/eliasdaler/imgui-sfml/archive/49dbecb43040449cccb3bfc43e3472cee94da417.tar.gz",
)

http_archive(
    name = "libpng",  # Libpng
    build_file = "//third_party:libpng.BUILD",
    sha256 = "a00e9d2f2f664186e4202db9299397f851aea71b36a35e74910b8820e380d441",
    strip_prefix = "libpng-1.6.39",
    url = "https://github.com/glennrp/libpng/archive/v1.6.39.tar.gz",
)

http_archive(
    name = "sfml",  # Zlib
    build_file = "//third_party:sfml.BUILD",
    # Work around SFML check for enough bytes for a given UTF-8 character crashing
    # in MSVC debug builds with "cannot seek string_view iterator after end".
    # See: https://github.com/SFML/SFML/issues/2113
    patch_cmds = [
        "sed -i 's/if (begin + trailingBytes < end)/if (trailingBytes < std::distance(begin, end))/' include/SFML/System/Utf.inl",
    ],
    sha256 = "6124b5fe3d96e7f681f587e2d5b456cd0ec460393dfe46691f1933d6bde0640b",
    strip_prefix = "SFML-2.5.1",
    url = "https://github.com/SFML/SFML/archive/2.5.1.zip",
)

# 1.11.0 + fixes for -Wshadow.
http_archive(
    name = "spdlog",  # MIT
    build_file = "//third_party:spdlog.BUILD",
    sha256 = "6be1b1c8ec9ed82b1f5c4d793678d5d9446e000eb38bb53c7d470365171722fd",
    strip_prefix = "spdlog-6df64c6c34997e298a81150b9ce55c440c6f503f",
    url = "https://github.com/gabime/spdlog/archive/6df64c6c34997e298a81150b9ce55c440c6f503f.tar.gz",
)

http_archive(
    name = "stb",  # MIT/Unlicense
    build_file = "//third_party:stb.BUILD",
    sha256 = "c47cf5abe21e1d620afccd159c23fe71dfa86eb270015a7646a4f79e9bfd5503",
    strip_prefix = "stb-8b5f1f37b5b75829fc72d38e7b5d4bcbf8a26d55",
    url = "https://github.com/nothings/stb/archive/8b5f1f37b5b75829fc72d38e7b5d4bcbf8a26d55.tar.gz",
)

http_archive(
    name = "udev-zero",  # ISC
    build_file = "//third_party:udev-zero.BUILD",
    sha256 = "c4cf149ea96295c1e6e86038d10c725344c751982ed4a790b06c76776923e0ea",
    strip_prefix = "libudev-zero-1.0.1",
    url = "https://github.com/illiliti/libudev-zero/archive/1.0.1.tar.gz",
)

http_archive(
    name = "vulkan",  # Apache-2.0
    build_file = "//third_party:vulkan.BUILD",
    sha256 = "4ee7c8e588e3e90785636c7ac879ad7d0b8d363c26838f57d3e4a7b8a55c38f5",
    strip_prefix = "Vulkan-Headers-1.3.242",
    url = "https://github.com/KhronosGroup/Vulkan-Headers/archive/v1.3.242.tar.gz",
)

http_archive(
    name = "xext",  # MIT
    build_file = "//third_party:xext.BUILD",
    sha256 = "dcf5fd6defbe474912fb6c617f8b926e53f828698c8491a8abab955ab071fc3f",
    strip_prefix = "libxext-libXext-1.3.5",
    url = "https://gitlab.freedesktop.org/xorg/lib/libxext/-/archive/libXext-1.3.5/libxext-libXext-1.3.5.tar.gz",
)

http_archive(
    name = "xrandr",  # MIT
    build_file = "//third_party:xrandr.BUILD",
    sha256 = "1b46bad15e13eb22db31abf4b3e73dfdc13f8c709897dcf39ce6ca6827c7a741",
    strip_prefix = "xorg-libXrandr-libXrandr-1.5.3",
    url = "https://github.com/freedesktop/xorg-libXrandr/archive/libXrandr-1.5.3.tar.gz",
)

http_archive(
    name = "xrender",  # MIT
    build_file = "//third_party:xrender.BUILD",
    sha256 = "9eaa3cc9f80d173b0d09937c56ca118701ed79dfa85cec334290ae53cf1a2e61",
    strip_prefix = "xorg-libXrender-libXrender-0.9.11",
    url = "https://github.com/freedesktop/xorg-libXrender/archive/libXrender-0.9.11.tar.gz",
)

http_archive(
    name = "zlib",  # Zlib
    build_file = "//third_party:zlib.BUILD",
    sha256 = "1525952a0a567581792613a9723333d7f8cc20b87a81f920fb8bc7e3f2251428",
    strip_prefix = "zlib-1.2.13",
    url = "https://github.com/madler/zlib/archive/v1.2.13.tar.gz",
)
