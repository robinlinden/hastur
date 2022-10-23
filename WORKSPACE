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

# HEAD as of 2022-09-06.
http_archive(
    name = "hedron_compile_commands",
    sha256 = "4b251a482a85de6c5cb0dc34c5671e73190b9ff348e9979fa2c033d81de0f928",
    strip_prefix = "bazel-compile-commands-extractor-5bb5ff2f32d542a986033102af771aa4206387b9",
    url = "https://github.com/hedronvision/bazel-compile-commands-extractor/archive/5bb5ff2f32d542a986033102af771aa4206387b9.tar.gz",
)

load("@hedron_compile_commands//:workspace_setup.bzl", "hedron_compile_commands_setup")

hedron_compile_commands_setup()

# Third-party
# =========================================================

http_archive(
    name = "asio",  # BSL-1.0
    build_file = "//third_party:asio.BUILD",
    sha256 = "cbcaaba0f66722787b1a7c33afe1befb3a012b5af3ad7da7ff0f6b8c9b7a8a5b",
    strip_prefix = "asio-asio-1-24-0",
    url = "https://github.com/chriskohlhoff/asio/archive/asio-1-24-0.tar.gz",
)

# boringssl//:ssl cheats and pulls in private includes from boringssl//:crypto.
http_archive(
    name = "boringssl",  # OpenSSL + ISC
    patch_cmds = ["sed -i '33i package(features=[\"-layering_check\"])' BUILD"],
    sha256 = "be8231e5f3b127d83eb156354dfa28c110e3c616c11ae119067c8184ef7a257f",
    strip_prefix = "boringssl-3a3d0b5c7fddeea312b5ce032d9b84a2be399b32",
    url = "https://github.com/google/boringssl/archive/3a3d0b5c7fddeea312b5ce032d9b84a2be399b32.tar.gz",
)

http_archive(
    name = "ctre",  # Apache-2.0
    build_file = "//third_party:ctre.BUILD",
    sha256 = "d00d7eaa0e22f2fdaa947a532b81b6fc35880acf4887b50a5ac9bfb7411ced03",
    strip_prefix = "compile-time-regular-expressions-3.7.1",
    url = "https://github.com/hanickadot/compile-time-regular-expressions/archive/v3.7.1.tar.gz",
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
    sha256 = "efe71fd4b8246f1b0b1b9bfca13cfff1c9ad85930340c27df469733bbb620938",
    strip_prefix = "freetype-2.12.1",
    url = "https://download.savannah.gnu.org/releases/freetype/freetype-2.12.1.tar.gz",
)

# 094d8d9d0a3cd19a7258a13d21ccb6acca60b858 contains a workaround for a Clang
# compiler crash that was affecting us on Windows w/ clang-cl.
http_archive(
    name = "ftxui",  # MIT
    build_file = "//third_party:ftxui.BUILD",
    sha256 = "2fbc119e30d0e236badf6136ac1b672284a861174cad10a7d336487148f08c0d",
    strip_prefix = "FTXUI-094d8d9d0a3cd19a7258a13d21ccb6acca60b858",
    url = "https://github.com/ArthurSonzogni/FTXUI/archive/094d8d9d0a3cd19a7258a13d21ccb6acca60b858.tar.gz",
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
    sha256 = "9f14c788aee15b777051e48f868c5d4d959bd679fc5050e3d2a29de80d8fd32e",
    strip_prefix = "imgui-1.88",
    url = "https://github.com/ocornut/imgui/archive/v1.88.tar.gz",
)

http_archive(
    name = "imgui-sfml",  # MIT
    build_file = "//third_party:imgui-sfml.BUILD",
    sha256 = "848315ec333c8d2e1f5cf0722408f2f5e2fb0de49936d1e9cd23c591193b8905",
    strip_prefix = "imgui-sfml-3dd9b4d35c7caf21c81231410e2c4785d40d418d",
    url = "https://github.com/eliasdaler/imgui-sfml/archive/3dd9b4d35c7caf21c81231410e2c4785d40d418d.tar.gz",
)

http_archive(
    name = "libpng",  # Libpng
    build_file = "//third_party:libpng.BUILD",
    sha256 = "d4160037fa5d09fa7cff555037f2a7f2fefc99ca01e21723b19bfcda33015234",
    strip_prefix = "libpng-1.6.38",
    url = "https://github.com/glennrp/libpng/archive/v1.6.38.tar.gz",
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

http_archive(
    name = "spdlog",  # MIT
    build_file = "//third_party:spdlog.BUILD",
    sha256 = "697f91700237dbae2326b90469be32b876b2b44888302afbc7aceb68bcfe8224",
    strip_prefix = "spdlog-1.10.0",
    url = "https://github.com/gabime/spdlog/archive/v1.10.0.tar.gz",
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
    sha256 = "fe620275ca1e29501dcb3f54c69cc011b6d9c3296408fac4e18dc491a1be754f",
    strip_prefix = "Vulkan-Headers-1.3.229",
    url = "https://github.com/KhronosGroup/Vulkan-Headers/archive/v1.3.229.tar.gz",
)

http_archive(
    name = "xext",  # MIT
    build_file = "//third_party:xext.BUILD",
    sha256 = "1a0f56d602100e320e553a799ef3fec626515bbe5e04f376bc44566d71dde288",
    strip_prefix = "libxext-libXext-1.3.4",
    url = "https://gitlab.freedesktop.org/xorg/lib/libxext/-/archive/libXext-1.3.4/libxext-libXext-1.3.4.tar.gz",
)

http_archive(
    name = "xrandr",  # MIT
    build_file = "//third_party:xrandr.BUILD",
    sha256 = "55cd6a2797cb79823b8a611dbc695d93262fd0d6a663d9f52422d7d25b81b4b1",
    strip_prefix = "xorg-libXrandr-libXrandr-1.5.2",
    url = "https://github.com/freedesktop/xorg-libXrandr/archive/libXrandr-1.5.2.tar.gz",
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
