load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("//bzl:github_archive.bzl", "github_archive")

# Bazel
# =========================================================

http_archive(
    name = "platforms",  # Apache-2.0
    sha256 = "5308fc1d8865406a49427ba24a9ab53087f17f5266a7aabbfc28823f3916e1ca",
    url = "https://github.com/bazelbuild/platforms/releases/download/0.0.6/platforms-0.0.6.tar.gz",
)

github_archive(
    name = "rules_fuzzing",
    build_file = None,
    repo = "bazelbuild/rules_fuzzing",
    sha256 = "f85dc70bb9672af0e350686461fe6fdd0d61e10e75645f9e44fedf549b21e369",
    version = "v0.3.2",
)

load("@rules_fuzzing//fuzzing:repositories.bzl", "rules_fuzzing_dependencies")

rules_fuzzing_dependencies()

load("@rules_fuzzing//fuzzing:init.bzl", "rules_fuzzing_init")

rules_fuzzing_init()

# Misc tools
# =========================================================

# HEAD as of 2022-12-17.
github_archive(
    name = "hedron_compile_commands",
    build_file = None,
    repo = "hedronvision/bazel-compile-commands-extractor",
    sha256 = "9b5683e6e0d764585f41639076f0be421a4c495c8f993c186e4449977ce03e5e",
    version = "c6cd079bef5836293ca18e55aac6ef05134c3a9d",
)

load("@hedron_compile_commands//:workspace_setup.bzl", "hedron_compile_commands_setup")

hedron_compile_commands_setup()

# Third-party
# =========================================================

github_archive(
    name = "asio",  # BSL-1.0
    repo = "chriskohlhoff/asio",
    sha256 = "cbcaaba0f66722787b1a7c33afe1befb3a012b5af3ad7da7ff0f6b8c9b7a8a5b",
    version = "asio-1-24-0",
)

# boringssl//:ssl cheats and pulls in private includes from boringssl//:crypto.
github_archive(
    name = "boringssl",  # OpenSSL + ISC
    build_file = None,
    patch_cmds = ["sed -i '33i package(features=[\"-layering_check\"])' BUILD"],
    repo = "google/boringssl",
    sha256 = "be8231e5f3b127d83eb156354dfa28c110e3c616c11ae119067c8184ef7a257f",
    version = "3a3d0b5c7fddeea312b5ce032d9b84a2be399b32",
)

github_archive(
    name = "ctre",  # Apache-2.0
    repo = "hanickadot/compile-time-regular-expressions",
    sha256 = "d00d7eaa0e22f2fdaa947a532b81b6fc35880acf4887b50a5ac9bfb7411ced03",
    version = "3.7.1",
)

github_archive(
    name = "fmt",  # MIT
    repo = "fmtlib/fmt",
    sha256 = "5dea48d1fcddc3ec571ce2058e13910a0d4a6bab4cc09a809d8b1dd1c88ae6f2",
    version = "9.1.0",
)

github_archive(
    name = "freetype2",  # FTL
    repo = "freetype/freetype",
    sha256 = "0e72cae32751598d126cfd4bceda909f646b7231ab8c52e28abb686c20a2bea1",
    version = "VER-2-12-1",
)

# 094d8d9d0a3cd19a7258a13d21ccb6acca60b858 contains a workaround for a Clang
# compiler crash that was affecting us on Windows w/ clang-cl.
github_archive(
    name = "ftxui",  # MIT
    repo = "ArthurSonzogni/FTXUI",
    sha256 = "2fbc119e30d0e236badf6136ac1b672284a861174cad10a7d336487148f08c0d",
    version = "094d8d9d0a3cd19a7258a13d21ccb6acca60b858",
)

github_archive(
    name = "glew",  # BSD-3-Clause
    repo = "nigels-com/glew",
    sha256 = "d4fc82893cfb00109578d0a1a2337fb8ca335b3ceccf97b97e5cc7f08e4353e1",
    version = "v2.2.0",
)

github_archive(
    name = "imgui",  # MIT
    repo = "ocornut/imgui",
    sha256 = "6d02a0079514d869e4b5f8f590f9060259385fcddd93a07ef21298b6a9610cbd",
    version = "v1.89.1",
)

github_archive(
    name = "imgui-sfml",  # MIT
    repo = "eliasdaler/imgui-sfml",
    sha256 = "c9f5f5ed92ad30afb64f32e2e0d4b4050c59de465f759330e972b90891798581",
    version = "49dbecb43040449cccb3bfc43e3472cee94da417",
)

github_archive(
    name = "libpng",  # Libpng
    repo = "glennrp/libpng",
    sha256 = "a00e9d2f2f664186e4202db9299397f851aea71b36a35e74910b8820e380d441",
    version = "v1.6.39",
)

github_archive(
    name = "sfml",  # Zlib
    # Work around SFML check for enough bytes for a given UTF-8 character crashing
    # in MSVC debug builds with "cannot seek string_view iterator after end".
    # See: https://github.com/SFML/SFML/issues/2113
    patch_cmds = [
        "sed -i 's/if (begin + trailingBytes < end)/if (trailingBytes < std::distance(begin, end))/' include/SFML/System/Utf.inl",
    ],
    repo = "SFML/SFML",
    sha256 = "438c91a917cc8aa19e82c6f59f8714da353c488584a007d401efac8368e1c785",
    version = "2.5.1",
)

github_archive(
    name = "spdlog",  # MIT
    repo = "gabime/spdlog",
    sha256 = "ca5cae8d6cac15dae0ec63b21d6ad3530070650f68076f3a4a862ca293a858bb",
    version = "v1.11.0",
)

github_archive(
    name = "stb",  # MIT/Unlicense
    repo = "nothings/stb",
    sha256 = "c47cf5abe21e1d620afccd159c23fe71dfa86eb270015a7646a4f79e9bfd5503",
    version = "8b5f1f37b5b75829fc72d38e7b5d4bcbf8a26d55",
)

github_archive(
    name = "udev-zero",  # ISC
    repo = "illiliti/libudev-zero",
    sha256 = "c4cf149ea96295c1e6e86038d10c725344c751982ed4a790b06c76776923e0ea",
    version = "v1.0.1",
)

github_archive(
    name = "vulkan",  # Apache-2.0
    repo = "KhronosGroup/Vulkan-Headers",
    sha256 = "fe620275ca1e29501dcb3f54c69cc011b6d9c3296408fac4e18dc491a1be754f",
    version = "1.3.229",
)

http_archive(
    name = "xext",  # MIT
    build_file = "//third_party:xext.BUILD",
    sha256 = "1a0f56d602100e320e553a799ef3fec626515bbe5e04f376bc44566d71dde288",
    strip_prefix = "libxext-libXext-1.3.4",
    url = "https://gitlab.freedesktop.org/xorg/lib/libxext/-/archive/libXext-1.3.4/libxext-libXext-1.3.4.tar.gz",
)

github_archive(
    name = "xrandr",  # MIT
    repo = "freedesktop/xorg-libXrandr",
    sha256 = "55cd6a2797cb79823b8a611dbc695d93262fd0d6a663d9f52422d7d25b81b4b1",
    version = "libXrandr-1.5.2",
)

github_archive(
    name = "xrender",  # MIT
    repo = "freedesktop/xorg-libXrender",
    sha256 = "9eaa3cc9f80d173b0d09937c56ca118701ed79dfa85cec334290ae53cf1a2e61",
    version = "libXrender-0.9.11",
)

github_archive(
    name = "zlib",  # Zlib
    repo = "madler/zlib",
    sha256 = "1525952a0a567581792613a9723333d7f8cc20b87a81f920fb8bc7e3f2251428",
    version = "v1.2.13",
)
