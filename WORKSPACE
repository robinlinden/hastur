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

# Misc tools
# =========================================================

# HEAD as of 2022-12-17.
http_archive(
    name = "hedron_compile_commands",
    sha256 = "9b5683e6e0d764585f41639076f0be421a4c495c8f993c186e4449977ce03e5e",
    strip_prefix = "bazel-compile-commands-extractor-c6cd079bef5836293ca18e55aac6ef05134c3a9d",
    url = "https://github.com/hedronvision/bazel-compile-commands-extractor/archive/c6cd079bef5836293ca18e55aac6ef05134c3a9d.tar.gz",
)

# Third-party
# =========================================================

# We don't use this, but rules_fuzzing does, and the version they depend on
# doesn't work with Clang 16 due to undeclared inclusions.
http_archive(
    name = "com_google_absl",  # Apache-2.0
    sha256 = "9a2b5752d7bfade0bdeee2701de17c9480620f8b237e1964c1b9967c75374906",
    strip_prefix = "abseil-cpp-20230125.2",
    url = "https://github.com/abseil/abseil-cpp/archive/20230125.2.tar.gz",
)

http_archive(
    name = "asio",  # BSL-1.0
    build_file = "//third_party:asio.BUILD",
    sha256 = "226438b0798099ad2a202563a83571ce06dd13b570d8fded4840dbc1f97fa328",
    strip_prefix = "asio-asio-1-28-0",
    url = "https://github.com/chriskohlhoff/asio/archive/asio-1-28-0.tar.gz",
)

# https://github.com/google/boringssl
# boringssl//:ssl cheats and pulls in private includes from boringssl//:crypto.
http_archive(
    name = "boringssl",  # OpenSSL + ISC
    patch_cmds = ["sed -i '33i package(features=[\"-layering_check\"])' BUILD"],
    sha256 = "62939a56062a3be7417a4e195c8bf0e7f07fdc26a9c49ef42ffd9b4030e3e921",
    strip_prefix = "boringssl-3c13ec0a400cfe5a29c5e0726cecaa51d2bffcc9",
    url = "https://github.com/google/boringssl/archive/3c13ec0a400cfe5a29c5e0726cecaa51d2bffcc9.tar.gz",
)

http_archive(
    name = "ctre",  # Apache-2.0
    build_file = "//third_party:ctre.BUILD",
    sha256 = "0711a6f97496e010f72adab69839939a9e50ba35ad87779e422ae3ff3b0edfc3",
    strip_prefix = "compile-time-regular-expressions-3.7.2",
    url = "https://github.com/hanickadot/compile-time-regular-expressions/archive/v3.7.2.tar.gz",
)

http_archive(
    name = "expected",  # CC0-1.0
    build_file = "//third_party:expected.BUILD",
    sha256 = "1db357f46dd2b24447156aaf970c4c40a793ef12a8a9c2ad9e096d9801368df6",
    strip_prefix = "expected-1.1.0",
    url = "https://github.com/tartanllama/expected/archive/v1.1.0.tar.gz",
)

http_archive(
    name = "fmt",  # MIT
    build_file = "//third_party:fmt.BUILD",
    sha256 = "ede1b6b42188163a3f2e0f25ad5c0637eca564bd8df74d02e31a311dd6b37ad8",
    strip_prefix = "fmt-10.0.0",
    url = "https://github.com/fmtlib/fmt/archive/10.0.0.tar.gz",
)

http_archive(
    name = "freetype2",  # FTL
    build_file = "//third_party:freetype2.BUILD",
    sha256 = "a683f1091aee95d2deaca9292d976f87415610b8ae1ea186abeebcb08e83ab12",
    strip_prefix = "freetype-VER-2-13-0",
    url = "https://github.com/freetype/freetype/archive/VER-2-13-0.tar.gz",
)

# https://github.com/ArthurSonzogni/FTXUI
http_archive(
    name = "ftxui",  # MIT
    build_file = "//third_party:ftxui.BUILD",
    sha256 = "9009d093e48b3189487d67fc3e375a57c7b354c0e43fc554ad31bec74a4bc2dd",
    strip_prefix = "FTXUI-4.1.1",
    url = "https://github.com/ArthurSonzogni/FTXUI/archive/v4.1.1.tar.gz",
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
    name = "icu-data",  # Unicode-DFS-2016
    build_file_content = "exports_files([\"icudt72l.dat\"])",
    sha256 = "1bc02487cbeaec3fc2d0dc941e8b243e7d35cd79899a201df88dc9ec9667a162",
    url = "https://github.com/unicode-org/icu/releases/download/release-72-1/icu4c-72_1-data-bin-l.zip",
)

# https://github.com/ocornut/imgui
http_archive(
    name = "imgui",  # MIT
    build_file = "//third_party:imgui.BUILD",
    sha256 = "e95d1cba1481e66386acda3e7da19cd738da86c6c2a140a48fa55046e5f6e208",
    strip_prefix = "imgui-1.89.6",
    url = "https://github.com/ocornut/imgui/archive/v1.89.6.tar.gz",
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

# 1.11.0 + fixes for -Wshadow + compatibility w/ fmt 10.0.0.
http_archive(
    name = "spdlog",  # MIT
    build_file = "//third_party:spdlog.BUILD",
    sha256 = "f516c1267a3f854d38d7fa8bc246e238a8c700a038ef4b2c2f669de8a9c7a25c",
    strip_prefix = "spdlog-0ca574ae168820da0268b3ec7607ca7b33024d05",
    url = "https://github.com/gabime/spdlog/archive/0ca574ae168820da0268b3ec7607ca7b33024d05.tar.gz",
)

http_archive(
    name = "stb",  # MIT/Unlicense
    build_file = "//third_party:stb.BUILD",
    sha256 = "d00921d49b06af62aa6bfb97c1b136bec661dd11dd4eecbcb0da1f6da7cedb4c",
    strip_prefix = "stb-5736b15f7ea0ffb08dd38af21067c314d6a3aae9",
    url = "https://github.com/nothings/stb/archive/5736b15f7ea0ffb08dd38af21067c314d6a3aae9.tar.gz",
)

http_archive(
    name = "udev-zero",  # ISC
    build_file = "//third_party:udev-zero.BUILD",
    sha256 = "29dff942cab9519994fb92ba6407f57e08d3dd6e6c0b86bb93d7b1d681994ff8",
    strip_prefix = "libudev-zero-1.0.2",
    url = "https://github.com/illiliti/libudev-zero/archive/1.0.2.tar.gz",
)

# https://github.com/facebookexperimental/libunifex
http_archive(
    name = "unifex",  # Apache-2.0 WITH LLVM-exception
    build_file = "//third_party:unifex.BUILD",
    sha256 = "e5780cfe8b6ffe64079e26a827a6893174361749f78545a899a34640f5ca3b75",
    strip_prefix = "libunifex-0.2.0",
    url = "https://github.com/facebookexperimental/libunifex/archive/v0.2.0.tar.gz",
)

VULKAN_TAG = "1.3.251"

# https://github.com/KhronosGroup/Vulkan-Headers
http_archive(
    name = "vulkan",  # Apache-2.0
    build_file = "//third_party:vulkan.BUILD",
    sha256 = "e14ac3a6868d9cffcd76e8e92eb0373eb675ab5725672af35b4ba664348e8261",
    strip_prefix = "Vulkan-Headers-%s" % VULKAN_TAG,
    url = "https://github.com/KhronosGroup/Vulkan-Headers/archive/v%s.tar.gz" % VULKAN_TAG,
)

# https://github.com/KhronosGroup/Vulkan-Hpp
http_archive(
    name = "vulkan_hpp",  # Apache-2.0
    build_file = "//third_party:vulkan_hpp.BUILD",
    sha256 = "068009952cb2e83e8d1c77a22abe1758f93ff3b45cb0cd6ba9951da67ad1fabf",
    strip_prefix = "Vulkan-Hpp-%s" % VULKAN_TAG,
    url = "https://github.com/KhronosGroup/Vulkan-Hpp/archive/v%s.tar.gz" % VULKAN_TAG,
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
    sha256 = "a58611b6de3932439ccf9330096e015925f1bd315d9b89a47297362b362fdbd8",
    strip_prefix = "libxrandr-libXrandr-1.5.3",
    url = "https://gitlab.freedesktop.org/xorg/lib/libxrandr/-/archive/libXrandr-1.5.3/libxrandr-libXrandr-1.5.3.tar.gz",
)

http_archive(
    name = "xrender",  # MIT
    build_file = "//third_party:xrender.BUILD",
    sha256 = "4cd5aca5b948a80bb7c3d5060eb97b8a8199234c0c19fe34d35c5c838923230b",
    strip_prefix = "libxrender-libXrender-0.9.11",
    url = "https://gitlab.freedesktop.org/xorg/lib/libxrender/-/archive/libXrender-0.9.11/libxrender-libXrender-0.9.11.tar.gz",
)

http_archive(
    name = "zlib",  # Zlib
    build_file = "//third_party:zlib.BUILD",
    sha256 = "1525952a0a567581792613a9723333d7f8cc20b87a81f920fb8bc7e3f2251428",
    strip_prefix = "zlib-1.2.13",
    url = "https://github.com/madler/zlib/archive/v1.2.13.tar.gz",
)

# Third-party setup
# =========================================================

# This needs to go last so that we can override any dependencies these calls may
# pull in.

load("@rules_fuzzing//fuzzing:repositories.bzl", "rules_fuzzing_dependencies")

rules_fuzzing_dependencies()

load("@rules_fuzzing//fuzzing:init.bzl", "rules_fuzzing_init")

rules_fuzzing_init()

load("@hedron_compile_commands//:workspace_setup.bzl", "hedron_compile_commands_setup")

hedron_compile_commands_setup()
