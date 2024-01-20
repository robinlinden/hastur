load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# Bazel
# =========================================================

# https://github.com/bazelbuild/apple_support
http_archive(
    name = "build_bazel_apple_support",
    sha256 = "cf4d63f39c7ba9059f70e995bf5fe1019267d3f77379c2028561a5d7645ef67c",
    url = "https://github.com/bazelbuild/apple_support/releases/download/1.11.1/apple_support.1.11.1.tar.gz",
)

# https://github.com/bazelbuild/platforms
http_archive(
    name = "platforms",  # Apache-2.0
    sha256 = "8150406605389ececb6da07cbcb509d5637a3ab9a24bc69b1101531367d89d74",
    url = "https://github.com/bazelbuild/platforms/releases/download/0.0.8/platforms-0.0.8.tar.gz",
)

# https://github.com/bazelbuild/rules_cc
http_archive(
    name = "rules_cc",  # Apache-2.0
    sha256 = "2037875b9a4456dce4a79d112a8ae885bbc4aad968e6587dca6e64f3a0900cdf",
    strip_prefix = "rules_cc-0.0.9",
    url = "https://github.com/bazelbuild/rules_cc/releases/download/0.0.9/rules_cc-0.0.9.tar.gz",
)

# https://github.com/bazelbuild/rules_fuzzing
# v0.4.1 + fix for rules_python 0.27.0 compatibility.
http_archive(
    name = "rules_fuzzing",
    sha256 = "484242608494d91e4c8e5efb2d748d3f0ec2c9346d31b107667afc5d81a0f8c2",
    strip_prefix = "rules_fuzzing-67ba0264c46c173a75825f2ae0a0b4b9b17c5e59",
    url = "https://github.com/bazelbuild/rules_fuzzing/archive/67ba0264c46c173a75825f2ae0a0b4b9b17c5e59.tar.gz",
)

# https://github.com/bazelbuild/rules_python
http_archive(
    name = "rules_python",  # Apache-2.0
    integrity = "sha256-1wzXKnpIgPAACmNGJTQUglwZzdQKKCib32e45kgO3/g=",
    strip_prefix = "rules_python-0.28.0",
    url = "https://github.com/bazelbuild/rules_python/releases/download/0.28.0/rules_python-0.28.0.tar.gz",
)

# Third-party Bazel
# =========================================================

# https://github.com/uber/hermetic_cc_toolchain
http_archive(
    name = "hermetic_cc_toolchain",
    integrity = "sha256-O4EH3g0Bf+MuZDQIapVo+XxgoRG0ncNPxwAeE5ww/eo=",
    url = "https://github.com/uber/hermetic_cc_toolchain/releases/download/v2.2.1/hermetic_cc_toolchain-v2.2.1.tar.gz",
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

# HEAD as of 2024-01-11.
# https://github.com/erenon/bazel_clang_tidy
http_archive(
    name = "bazel_clang_tidy",
    integrity = "sha256-CWFhUEXpdtbJi1PLDfe8hZOQiKyv6QV2Xi9wnKMVDyM=",
    strip_prefix = "bazel_clang_tidy-f43f9d61c201b314c62a3ebcf2d4a37f1a3b06f7",
    url = "https://github.com/erenon/bazel_clang_tidy/archive/f43f9d61c201b314c62a3ebcf2d4a37f1a3b06f7.tar.gz",
)

# Third-party
# =========================================================

# https://github.com/chriskohlhoff/asio
http_archive(
    name = "asio",  # BSL-1.0
    build_file = "//third_party:asio.BUILD",
    integrity = "sha256-RDBYWbTmZk27+FPB74ygJZ1pTwM3U64wn8slNMog9yE=",
    strip_prefix = "asio-asio-1-29-0",
    url = "https://github.com/chriskohlhoff/asio/archive/asio-1-29-0.tar.gz",
)

# https://github.com/google/boringssl
http_archive(
    name = "boringssl",  # OpenSSL + ISC
    integrity = "sha256-tfQKXmPxOqN8wsCU2woTfB0mfme/soi5t2IlLLvz5E4=",
    # boringssl//:ssl cheats and pulls in private includes from boringssl//:crypto.
    patch_cmds = ["""sed -i'' -e '33i\\\npackage(features=["-layering_check"])' BUILD"""],
    strip_prefix = "boringssl-2cc231074e7437987f1b9621b5d1936ba4d092ae",
    url = "https://github.com/google/boringssl/archive/2cc231074e7437987f1b9621b5d1936ba4d092ae.tar.gz",
)

http_archive(
    name = "expected",  # CC0-1.0
    build_file = "//third_party:expected.BUILD",
    sha256 = "1db357f46dd2b24447156aaf970c4c40a793ef12a8a9c2ad9e096d9801368df6",
    strip_prefix = "expected-1.1.0",
    url = "https://github.com/tartanllama/expected/archive/v1.1.0.tar.gz",
)

# https://github.com/fmtlib/fmt
http_archive(
    name = "fmt",  # MIT
    build_file = "//third_party:fmt.BUILD",
    integrity = "sha256-ElDkzFi/Bu5jFWdSP0iEjcRZYTPhY/AmFcl/eLq2yBE=",
    strip_prefix = "fmt-10.2.1",
    url = "https://github.com/fmtlib/fmt/archive/10.2.1.tar.gz",
)

# https://github.com/freetype/freetype
http_archive(
    name = "freetype2",  # FTL
    build_file = "//third_party:freetype2.BUILD",
    sha256 = "427201f5d5151670d05c1f5b45bef5dda1f2e7dd971ef54f0feaaa7ffd2ab90c",
    strip_prefix = "freetype-VER-2-13-2",
    url = "https://github.com/freetype/freetype/archive/VER-2-13-2.tar.gz",
)

# https://github.com/ArthurSonzogni/FTXUI
http_archive(
    name = "ftxui",  # MIT
    build_file = "//third_party:ftxui.BUILD",
    sha256 = "a2991cb222c944aee14397965d9f6b050245da849d8c5da7c72d112de2786b5b",
    strip_prefix = "FTXUI-5.0.0",
    url = "https://github.com/ArthurSonzogni/FTXUI/archive/v5.0.0.tar.gz",
)

# https://github.com/Dav1dde/glad/
local_repository(
    name = "glad",  # MIT
    path = "third_party/glad",
)

# https://github.com/unicode-org/icu
http_archive(
    name = "icu",  # Unicode-DFS-2016
    build_file = "//third_party:icu.BUILD",
    integrity = "sha256-ykZL+nO8AOvbhQVGUU0B86mDFZ/aD3aC/2v0095WhEw=",
    patch_cmds = [
        "rm source/common/BUILD.bazel",
        "rm source/stubdata/BUILD.bazel",
        "rm source/tools/toolutil/BUILD.bazel",
        "rm source/i18n/BUILD.bazel",
    ],
    strip_prefix = "icu-release-74-1/icu4c",
    url = "https://github.com/unicode-org/icu/archive/refs/tags/release-74-1.tar.gz",
)

# https://github.com/ocornut/imgui
http_archive(
    name = "imgui",  # MIT
    build_file = "//third_party:imgui.BUILD",
    integrity = "sha256-IdzJhbsq6P5IBHyGE128Q41pgKjy4IurvaW+ggWS8oI=",
    strip_prefix = "imgui-1.90.1",
    url = "https://github.com/ocornut/imgui/archive/v1.90.1.tar.gz",
)

# https://github.com/SFML/imgui-sfml
http_archive(
    name = "imgui-sfml",  # MIT
    build_file = "//third_party:imgui-sfml.BUILD",
    patch_cmds = [
        # Use glad for OpenGL instead of the system OpenGL headers.
        "sed -i'' -e /OpenGL.hpp/d imgui-SFML.cpp",
        "sed -i'' -e '4i\\\n#include <glad/gl.h>' imgui-SFML.cpp",
    ],
    sha256 = "b1195ca1210dd46c8049cfc8cae7f31cd34f1591da7de1c56297b277ac9c5cc0",
    strip_prefix = "imgui-sfml-2.6",
    url = "https://github.com/eliasdaler/imgui-sfml/archive/v2.6.tar.gz",
)

# https://github.com/simdjson/simdjson
http_archive(
    name = "simdjson",  # Apache-2.0
    build_file = "//third_party:simdjson.BUILD",
    integrity = "sha256-r9ggHPsavpJ3N9h2RBux8hcwqe5geKG4xhdOZGOYH6M=",
    strip_prefix = "simdjson-3.6.3",
    url = "https://github.com/simdjson/simdjson/archive/refs/tags/v3.6.3.tar.gz",
)

# https://github.com/glennrp/libpng
http_archive(
    name = "libpng",  # Libpng
    build_file = "//third_party:libpng.BUILD",
    sha256 = "62d25af25e636454b005c93cae51ddcd5383c40fa14aa3dae8f6576feb5692c2",
    strip_prefix = "libpng-1.6.40",
    url = "https://github.com/glennrp/libpng/archive/v1.6.40.tar.gz",
)

http_archive(
    name = "sfml",  # Zlib
    build_file = "//third_party:sfml.BUILD",
    # Work around SFML check for enough bytes for a given UTF-8 character crashing
    # in MSVC debug builds with "cannot seek string_view iterator after end".
    # See: https://github.com/SFML/SFML/issues/2113
    patch_cmds = [
        "sed -i'' -e 's/if (begin + trailingBytes < end)/if (trailingBytes < std::distance(begin, end))/' include/SFML/System/Utf.inl",
    ],
    sha256 = "6124b5fe3d96e7f681f587e2d5b456cd0ec460393dfe46691f1933d6bde0640b",
    strip_prefix = "SFML-2.5.1",
    url = "https://github.com/SFML/SFML/archive/2.5.1.zip",
)

# https://github.com/gabime/spdlog
http_archive(
    name = "spdlog",  # MIT
    build_file = "//third_party:spdlog.BUILD",
    sha256 = "4dccf2d10f410c1e2feaff89966bfc49a1abb29ef6f08246335b110e001e09a9",
    strip_prefix = "spdlog-1.12.0",
    url = "https://github.com/gabime/spdlog/archive/v1.12.0.tar.gz",
)

# https://github.com/nothings/stb
http_archive(
    name = "stb",  # MIT/Unlicense
    build_file = "//third_party:stb.BUILD",
    integrity = "sha256-qOUH+YfIEv6H8SLwq03DBJkF+HCEuoU3XvlySPx+YyY=",
    strip_prefix = "stb-f4a71b13373436a2866c5d68f8f80ac6f0bc1ffe",
    url = "https://github.com/nothings/stb/archive/f4a71b13373436a2866c5d68f8f80ac6f0bc1ffe.tar.gz",
)

# https://github.com/illiliti/libudev-zero
http_archive(
    name = "udev-zero",  # ISC
    build_file = "//third_party:udev-zero.BUILD",
    sha256 = "0bd89b657d62d019598e6c7ed726ff8fed80e8ba092a83b484d66afb80b77da5",
    strip_prefix = "libudev-zero-1.0.3",
    url = "https://github.com/illiliti/libudev-zero/archive/1.0.3.tar.gz",
)

# https://github.com/facebookexperimental/libunifex
http_archive(
    name = "unifex",  # Apache-2.0 WITH LLVM-exception
    build_file = "//third_party:unifex.BUILD",
    sha256 = "d5ce3b616e166da31e6b4284764a1feeba52aade868bcbffa94cfd86b402716e",
    strip_prefix = "libunifex-0.4.0",
    url = "https://github.com/facebookexperimental/libunifex/archive/v0.4.0.tar.gz",
)

VULKAN_TAG = "1.3.275"

# https://github.com/KhronosGroup/Vulkan-Headers
http_archive(
    name = "vulkan",  # Apache-2.0
    build_file = "//third_party:vulkan.BUILD",
    integrity = "sha256-cWHaZF29M/1Oph7sCODXc4mmQAEKy/SvwAI0+E35sxQ=",
    strip_prefix = "Vulkan-Headers-%s" % VULKAN_TAG,
    url = "https://github.com/KhronosGroup/Vulkan-Headers/archive/v%s.tar.gz" % VULKAN_TAG,
)

# https://github.com/KhronosGroup/Vulkan-Hpp
http_archive(
    name = "vulkan_hpp",  # Apache-2.0
    build_file = "//third_party:vulkan_hpp.BUILD",
    integrity = "sha256-9eZ9kJeSlmZKcz4U797a4hmNwR+Ij3VpuEv3F3XXmQk=",
    strip_prefix = "Vulkan-Hpp-%s" % VULKAN_TAG,
    url = "https://github.com/KhronosGroup/Vulkan-Hpp/archive/v%s.tar.gz" % VULKAN_TAG,
)

# https://github.com/web-platform-tests/wpt
http_archive(
    name = "wpt",  # BSD-3-Clause
    build_file_content = """exports_files(["url/resources/urltestdata.json"])""",
    sha256 = "2e7d1f7a15735c96e426bad5919183d903e350bd057a4acaf7062ea5b9f21990",
    strip_prefix = "wpt-merge_pr_42906",
    url = "https://github.com/web-platform-tests/wpt/archive/refs/tags/merge_pr_42906.tar.gz",
)

# The freedesktop GitLab goes down too often to be trusted.
http_archive(
    name = "xext",  # MIT
    build_file = "//third_party:xext.BUILD",
    sha256 = "dcf5fd6defbe474912fb6c617f8b926e53f828698c8491a8abab955ab071fc3f",
    strip_prefix = "libxext-libXext-1.3.5",
    urls = [
        "https://gitlab.freedesktop.org/xorg/lib/libxext/-/archive/libXext-1.3.5/libxext-libXext-1.3.5.tar.gz",
        "https://github.com/gitlab-freedesktop-mirrors/libxext/archive/refs/tags/libXext-1.3.5.tar.gz",
    ],
)

# https://gitlab.freedesktop.org/xorg/lib/libxrandr
http_archive(
    name = "xrandr",  # MIT
    build_file = "//third_party:xrandr.BUILD",
    sha256 = "a1909cbe9ded94187b6420ae8c347153f8278955265cb80a64cdae5501433396",
    strip_prefix = "libxrandr-libXrandr-1.5.4",
    urls = [
        "https://gitlab.freedesktop.org/xorg/lib/libxrandr/-/archive/libXrandr-1.5.4/libxrandr-libXrandr-1.5.4.tar.gz",
        "https://github.com/gitlab-freedesktop-mirrors/libxrandr/archive/libXrandr-1.5.4.tar.gz",
    ],
)

http_archive(
    name = "xrender",  # MIT
    build_file = "//third_party:xrender.BUILD",
    sha256 = "4cd5aca5b948a80bb7c3d5060eb97b8a8199234c0c19fe34d35c5c838923230b",
    strip_prefix = "libxrender-libXrender-0.9.11",
    urls = [
        "https://gitlab.freedesktop.org/xorg/lib/libxrender/-/archive/libXrender-0.9.11/libxrender-libXrender-0.9.11.tar.gz",
        "https://github.com/gitlab-freedesktop-mirrors/libxrender/archive/refs/tags/libXrender-0.9.11.tar.gz",
    ],
)

http_archive(
    name = "zlib",  # Zlib
    build_file = "//third_party:zlib.BUILD",
    sha256 = "b5b06d60ce49c8ba700e0ba517fa07de80b5d4628a037f4be8ad16955be7a7c0",
    strip_prefix = "zlib-1.3",
    url = "https://github.com/madler/zlib/archive/v1.3.tar.gz",
)

# Third-party setup
# =========================================================

# This needs to go last so that we can override any dependencies these calls may
# pull in.

# build_bazel_apple_support
load("@build_bazel_apple_support//lib:repositories.bzl", "apple_support_dependencies")

apple_support_dependencies()

# rules_python
load("@rules_python//python:repositories.bzl", "py_repositories", "python_register_toolchains")

py_repositories()

python_register_toolchains(
    name = "python_3_12",
    # Running the build as root works, but leads to cache-misses for .pyc files.
    ignore_root_user_error = True,
    python_version = "3.12.0",
)

load("@python_3_12//:defs.bzl", "interpreter")
load("@rules_python//python:pip.bzl", "pip_parse")

pip_parse(
    name = "pypi",
    python_interpreter_target = interpreter,
    requirements_lock = "//third_party:requirements.txt",
)

load("@pypi//:requirements.bzl", pypi_install_deps = "install_deps")

pypi_install_deps()

# rules_fuzzing
# Must be after rules_python due to not calling py_repositories when it should.
load("@rules_fuzzing//fuzzing:repositories.bzl", "rules_fuzzing_dependencies")

rules_fuzzing_dependencies()

load("@rules_fuzzing//fuzzing:init.bzl", "rules_fuzzing_init")

rules_fuzzing_init()

load("@fuzzing_py_deps//:requirements.bzl", fuzzing_py_deps_install_deps = "install_deps")

fuzzing_py_deps_install_deps()

# hermetic_cc_toolchain
load("@hermetic_cc_toolchain//toolchain:defs.bzl", zig_toolchains = "toolchains")

zig_toolchains()

# hedron_compile_commands
load("@hedron_compile_commands//:workspace_setup.bzl", "hedron_compile_commands_setup")

hedron_compile_commands_setup()
