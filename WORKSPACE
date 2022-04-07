load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# Bazel
# =========================================================

http_archive(
    name = "platforms",  # Apache-2.0
    sha256 = "379113459b0feaf6bfbb584a91874c065078aa673222846ac765f86661c27407",
    url = "https://github.com/bazelbuild/platforms/releases/download/0.0.5/platforms-0.0.5.tar.gz",
)

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
    sha256 = "6f2b0390dc23be79268da435b276b0ecfffd1adeaf9868d6a68860f9b9adbcb7",
    strip_prefix = "boringssl-ae0ce154470dc7d1e3073ba8adb1ef2b669c6471",
    url = "https://github.com/google/boringssl/archive/ae0ce154470dc7d1e3073ba8adb1ef2b669c6471.tar.gz",
)

http_archive(
    name = "fmt",  # MIT
    build_file = "//third_party:fmt.BUILD",
    sha256 = "3d794d3cf67633b34b2771eb9f073bde87e846e0d395d254df7b211ef1ec7346",
    strip_prefix = "fmt-8.1.1",
    url = "https://github.com/fmtlib/fmt/archive/8.1.1.tar.gz",
)

http_archive(
    name = "freetype2",  # FTL
    build_file = "//third_party:freetype2.BUILD",
    sha256 = "f8db94d307e9c54961b39a1cc799a67d46681480696ed72ecf78d4473770f09b",
    strip_prefix = "freetype-2.11.1",
    url = "https://download.savannah.gnu.org/releases/freetype/freetype-2.11.1.tar.gz",
)

http_archive(
    name = "ftxui",  # MIT
    build_file = "//third_party:ftxui.BUILD",
    sha256 = "d891695ef22176f0c09f8261a37af9ad5b262dd670a81e6b83661a23abc2c54f",
    strip_prefix = "FTXUI-2.0.0",
    url = "https://github.com/ArthurSonzogni/FTXUI/archive/v2.0.0.tar.gz",
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
    sha256 = "b54ceb35bda38766e36b87c25edf7a1cd8fd2cb8c485b245aedca6fb85645a20",
    strip_prefix = "imgui-1.87",
    url = "https://github.com/ocornut/imgui/archive/v1.87.tar.gz",
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
    sha256 = "15050e9748633484957a166150f680a0ba8030074db599aad7c2d432191712af",
    strip_prefix = "Vulkan-Headers-1.3.208",
    url = "https://github.com/KhronosGroup/Vulkan-Headers/archive/v1.3.208.tar.gz",
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
    patch_cmds = ["sed -i '/config/d' src/Xrenderint.h"],
    sha256 = "8be927e04cf7bc5a7ce3af24dc6905e05fcf29142f17304b1f4d224a2ca350b1",
    strip_prefix = "xorg-libXrender-libXrender-0.9.10",
    url = "https://github.com/freedesktop/xorg-libXrender/archive/libXrender-0.9.10.tar.gz",
)

http_archive(
    name = "zlib",  # Zlib
    build_file = "//third_party:zlib.BUILD",
    sha256 = "629380c90a77b964d896ed37163f5c3a34f6e6d897311f1df2a7016355c45eff",
    strip_prefix = "zlib-1.2.11",
    url = "https://github.com/madler/zlib/archive/v1.2.11.tar.gz",
)
