load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# Bazel
# =========================================================

http_archive(
    name = "platforms",  # Apache-2.0
    sha256 = "5308fc1d8865406a49427ba24a9ab53087f17f5266a7aabbfc28823f3916e1ca",
    url = "https://github.com/bazelbuild/platforms/releases/download/0.0.6/platforms-0.0.6.tar.gz",
)

# Misc tools
# =========================================================

http_archive(
    name = "hedron_compile_commands",
    sha256 = "89cf5a306d25ab14559c95e82d0237638a01eb45e8f4f181304540f97e4d66fe",
    strip_prefix = "bazel-compile-commands-extractor-d3cbc6220320e8d2fce856d8487b45e639e57758",
    url = "https://github.com/hedronvision/bazel-compile-commands-extractor/archive/d3cbc6220320e8d2fce856d8487b45e639e57758.tar.gz",
)

load("@hedron_compile_commands//:workspace_setup.bzl", "hedron_compile_commands_setup")

hedron_compile_commands_setup()

# Third-party
# =========================================================

http_archive(
    name = "asio",  # BSL-1.0
    build_file = "//third_party:asio.BUILD",
    sha256 = "6874d81a863d800ee53456b1cafcdd1abf38bbbf54ecf295056b053c0d7115ce",
    strip_prefix = "asio-1.22.1",
    url = "https://downloads.sourceforge.net/project/asio/asio/1.22.1%20(Stable)/asio-1.22.1.tar.bz2",
)

# boringssl//:ssl cheats and pulls in private includes from boringssl//:crypto.
http_archive(
    name = "boringssl",  # OpenSSL + ISC
    patch_cmds = ["sed -i '33i package(features=[\"-layering_check\"])' BUILD"],
    sha256 = "641c62d698e88d838fc8076098645b72ae3dc0ecb791b75282d6618ac424f4b2",
    strip_prefix = "boringssl-80692b63910ff9f3971412ea509449f73a114e18",
    url = "https://github.com/google/boringssl/archive/80692b63910ff9f3971412ea509449f73a114e18.tar.gz",
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
    sha256 = "ca74a0dace179a8422187671aee97dd3892b53e168627145271cad5b5ac81307",
    strip_prefix = "libpng-1.6.37",
    url = "https://github.com/glennrp/libpng/archive/v1.6.37.tar.gz",
)

http_archive(
    name = "range-v3",  # BSL-1.0
    sha256 = "015adb2300a98edfceaf0725beec3337f542af4915cec4d0b89fa0886f4ba9cb",
    strip_prefix = "range-v3-0.12.0",
    url = "https://github.com/ericniebler/range-v3/archive/0.12.0.tar.gz",
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
    sha256 = "936b4e506b5f55db178207e528ecdf5a411f67431447767d06c9b7061765cd7e",
    strip_prefix = "stb-af1a5bc352164740c1cc1354942b1c6b72eacb8a",
    url = "https://github.com/nothings/stb/archive/af1a5bc352164740c1cc1354942b1c6b72eacb8a.tar.gz",
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
    sha256 = "004b4f7841bd912d1cc3f5ac5694d5dea2f944f10053451233b533874368df87",
    strip_prefix = "Vulkan-Headers-1.3.217",
    url = "https://github.com/KhronosGroup/Vulkan-Headers/archive/v1.3.217.tar.gz",
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

# 0.9.10 + commit adding missing config.h include guard.
http_archive(
    name = "xrender",  # MIT
    build_file = "//third_party:xrender.BUILD",
    sha256 = "183ad84bd4a3b5460829cdabae4db7b12a3853554f6c7b085df3885de1e41209",
    strip_prefix = "xorg-libXrender-e314946813bcb96e8baedc1a290c48a2aa6ef162",
    url = "https://github.com/freedesktop/xorg-libXrender/archive/e314946813bcb96e8baedc1a290c48a2aa6ef162.tar.gz",
)

http_archive(
    name = "zlib",  # Zlib
    build_file = "//third_party:zlib.BUILD",
    sha256 = "d8688496ea40fb61787500e863cc63c9afcbc524468cedeb478068924eb54932",
    strip_prefix = "zlib-1.2.12",
    url = "https://github.com/madler/zlib/archive/v1.2.12.tar.gz",
)
