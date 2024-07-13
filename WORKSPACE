load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive", "http_file")
load("@bazel_tools//tools/build_defs/repo:local.bzl", "local_repository")

# Bazel
# =========================================================

# https://github.com/bazelbuild/apple_support
http_archive(
    name = "build_bazel_apple_support",
    integrity = "sha256-xLsrc2fEhDgjAK7nW+WYuS+EeJb7MbvSLzojRq32aoA=",
    url = "https://github.com/bazelbuild/apple_support/releases/download/1.15.1/apple_support.1.15.1.tar.gz",
)

# https://github.com/bazelbuild/platforms
http_archive(
    name = "platforms",  # Apache-2.0
    integrity = "sha256-IY7+juc20mo1cmY7N0olPAErcW2K8MB+hC6C8jigp+4=",
    url = "https://github.com/bazelbuild/platforms/releases/download/0.0.10/platforms-0.0.10.tar.gz",
)

# https://github.com/bazelbuild/rules_cc
http_archive(
    name = "rules_cc",  # Apache-2.0
    sha256 = "2037875b9a4456dce4a79d112a8ae885bbc4aad968e6587dca6e64f3a0900cdf",
    strip_prefix = "rules_cc-0.0.9",
    url = "https://github.com/bazelbuild/rules_cc/releases/download/0.0.9/rules_cc-0.0.9.tar.gz",
)

# https://github.com/bazelbuild/rules_fuzzing
http_archive(
    name = "rules_fuzzing",
    integrity = "sha256-5rwhm/rJ4fg7Mn3QkPcoqflz7pm5tdjloYSicy7whiM=",
    strip_prefix = "rules_fuzzing-0.5.2",
    url = "https://github.com/bazelbuild/rules_fuzzing/releases/download/v0.5.2/rules_fuzzing-0.5.2.zip",
)

# https://github.com/bazelbuild/rules_python
http_archive(
    name = "rules_python",  # Apache-2.0
    integrity = "sha256-4/HMegTZsJY1r7MTBzHtgrX1jq3IIz1O+1mUTZL/wG8=",
    strip_prefix = "rules_python-0.33.2",
    url = "https://github.com/bazelbuild/rules_python/releases/download/0.33.2/rules_python-0.33.2.tar.gz",
)

# Third-party Bazel
# =========================================================

# https://github.com/uber/hermetic_cc_toolchain
http_archive(
    name = "hermetic_cc_toolchain",
    integrity = "sha256-3wka/CXXOwlI7TcdPWG+7ylEf2kFCOArwk5wAczBLTg=",
    url = "https://github.com/uber/hermetic_cc_toolchain/releases/download/v3.1.0/hermetic_cc_toolchain-v3.1.0.tar.gz",
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

# HEAD as of 2024-02-07.
# https://github.com/erenon/bazel_clang_tidy
http_archive(
    name = "bazel_clang_tidy",
    integrity = "sha256-4+IyNvq1UkNPyFNcHTWjAJEi0Nl6CGJQcrdF1JzOe0Y=",
    strip_prefix = "bazel_clang_tidy-43bef6852a433f3b2a6b001daecc8bc91d791b92",
    url = "https://github.com/erenon/bazel_clang_tidy/archive/43bef6852a433f3b2a6b001daecc8bc91d791b92.tar.gz",
)

# Third-party
# =========================================================

# https://github.com/chriskohlhoff/asio
http_archive(
    name = "asio",  # BSL-1.0
    build_file = "//third_party:asio.BUILD",
    integrity = "sha256-dVvX+FpLJpxnrg6iVJB8B41AjM6OGjUq0u1mTSM3gOg=",
    strip_prefix = "asio-asio-1-30-2",
    url = "https://github.com/chriskohlhoff/asio/archive/asio-1-30-2.tar.gz",
)

# TODO(robinlinden): Broken as of 15cf0c04e817c6d3950030bc381db82044b6175c.
#                    We're stuck on the commit before that for now.
# https://github.com/google/boringssl
http_archive(
    name = "boringssl",  # OpenSSL + ISC
    integrity = "sha256-UuUvjBHbJifybzBvYK/3qSIlc9UzvrjT0VRRNRBTK9E=",
    patch_cmds = [
        # boringssl//:ssl cheats and pulls in private includes from boringssl//:crypto.
        """sed -i'' -e '33i\\\npackage(features=["-layering_check"])' BUILD""",
        # boringssl tries to use _Generic in C++ code.
        """sed -i'' -e 's/#if OPENSSL_HAS_BUILTIN(__builtin_addc)/& \\&\\& !defined(__cplusplus)/g' src/crypto/internal.h""",
        """sed -i'' -e 's/#if OPENSSL_HAS_BUILTIN(__builtin_subc)/& \\&\\& !defined(__cplusplus)/g' src/crypto/internal.h""",
    ],
    strip_prefix = "boringssl-c0534bb964f085e4e2f273d23d08e9585e7518aa",
    url = "https://github.com/google/boringssl/archive/c0534bb964f085e4e2f273d23d08e9585e7518aa.tar.gz",
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
    integrity = "sha256-fQCff4mshMCoP3ntYCRj0JL79mdjdmqQfJf9ArEA9ek=",
    strip_prefix = "fmt-11.0.1",
    url = "https://github.com/fmtlib/fmt/archive/11.0.1.tar.gz",
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
    patch_cmds = [
        # Work around circular header dependency detected by clang-tidy 18.
        """sed -i'' -e /deprecated.hpp/d include/ftxui/dom/elements.hpp""",
    ],
    sha256 = "a2991cb222c944aee14397965d9f6b050245da849d8c5da7c72d112de2786b5b",
    strip_prefix = "FTXUI-5.0.0",
    url = "https://github.com/ArthurSonzogni/FTXUI/archive/v5.0.0.tar.gz",
)

# https://github.com/Dav1dde/glad/
local_repository(
    name = "glad",  # MIT
    path = "third_party/glad",
)

# HEAD as of 2024-05-01.
# https://github.com/html5lib/html5lib-tests/
http_archive(
    name = "html5lib-tests",  # MIT
    build_file = "//third_party:html5lib-tests.BUILD",
    integrity = "sha256-mUhh8tSAqB9amozHbhgHVKqcDWEQe0dhiWbaUm8bFI0=",
    strip_prefix = "html5lib-tests-a9f44960a9fedf265093d22b2aa3c7ca123727b9",
    url = "https://github.com/html5lib/html5lib-tests/archive/a9f44960a9fedf265093d22b2aa3c7ca123727b9.tar.gz",
)

# https://github.com/unicode-org/icu
http_archive(
    name = "icu",  # Unicode-DFS-2016
    build_file = "//third_party:icu.BUILD",
    integrity = "sha256-kl5rS4z4hW4KwhT2804w3uY7e7elBGCrRgOVDv9I+J4=",
    patch_cmds = [
        "rm source/common/BUILD.bazel",
        "rm source/stubdata/BUILD.bazel",
        "rm source/tools/toolutil/BUILD.bazel",
        "rm source/i18n/BUILD.bazel",
    ],
    strip_prefix = "icu-release-75-1/icu4c",
    url = "https://github.com/unicode-org/icu/archive/refs/tags/release-75-1.tar.gz",
)

# https://www.unicode.org/Public/idna/
http_file(
    name = "idna_mapping_table",
    sha256 = "402cbd285f1f952fcd0834b63541d54f69d3d8f1b8f8599bf71a1a14935f82c4",
    url = "https://www.unicode.org/Public/idna/15.1.0/IdnaMappingTable.txt",
)

# https://github.com/ocornut/imgui
http_archive(
    name = "imgui",  # MIT
    build_file = "//third_party:imgui.BUILD",
    integrity = "sha256-BJQ5GXIeh0rHWi9F5utsAiQ5UDRme/UIkjOIr9paUL8=",
    strip_prefix = "imgui-1.90.9",
    url = "https://github.com/ocornut/imgui/archive/v1.90.9.tar.gz",
)

# https://github.com/SFML/imgui-sfml
http_archive(
    name = "imgui-sfml",  # MIT
    build_file = "//third_party:imgui-sfml.BUILD",
    patch_cmds = [
        # Use glad for OpenGL instead of the system OpenGL headers.
        "sed -i'' -e /OpenGL.hpp/d imgui-SFML.cpp",
        "sed -i'' -e '4i\\\n#include <glad/gl.h>' imgui-SFML.cpp",
        "sed -i'' -e '277i\\\n\\\tif (gladLoaderLoadGL() == 0) return false;' imgui-SFML.cpp",
    ],
    sha256 = "b1195ca1210dd46c8049cfc8cae7f31cd34f1591da7de1c56297b277ac9c5cc0",
    strip_prefix = "imgui-sfml-2.6",
    url = "https://github.com/SFML/imgui-sfml/archive/v2.6.tar.gz",
)

# https://github.com/simdjson/simdjson
http_archive(
    name = "simdjson",  # Apache-2.0
    build_file = "//third_party:simdjson.BUILD",
    integrity = "sha256-PZ6JSg3thcAA7Ftu0UUKtLA39EupXrOw/vtM2nLNnas=",
    strip_prefix = "simdjson-3.9.5",
    url = "https://github.com/simdjson/simdjson/archive/refs/tags/v3.9.5.tar.gz",
)

# https://github.com/glennrp/libpng
http_archive(
    name = "libpng",  # Libpng
    build_file = "//third_party:libpng.BUILD",
    integrity = "sha256-/syVtGzwXo4/yKQUdQ4LparQDYnp/fF16U/wQcrxoDo=",
    strip_prefix = "libpng-1.6.43",
    url = "https://github.com/glennrp/libpng/archive/v1.6.43.tar.gz",
)

# https://github.com/SFML/SFML
http_archive(
    name = "sfml",  # Zlib
    build_file = "//third_party:sfml.BUILD",
    integrity = "sha256-glNdueVxBdTzqK7avROGMd767cWTyrWJySS316Ef+50=",
    patch_cmds = [
        # Work around SFML check for enough bytes for a given UTF-8 character
        # crashing in MSVC debug builds with "cannot seek string_view iterator
        # after end".
        # See: https://github.com/SFML/SFML/issues/2113
        "sed -i'' -e 's/if (begin + trailingBytes < end)/if (trailingBytes < std::distance(begin, end))/' include/SFML/System/Utf.inl",
        # SFML uses a non-standard include path to vulkan.h
        # libvulkan-dev: /usr/include/vulkan/vulkan.h
        "sed -i'' -e 's|vulkan.h|vulkan/vulkan.h|' src/SFML/Window/Win32/VulkanImplWin32.cpp",
        "sed -i'' -e 's|vulkan.h|vulkan/vulkan.h|' src/SFML/Window/Unix/VulkanImplX11.cpp",
        # SFML does non-standard things with std::basic_string<Uint32>.
        # This will not compile with libc++19.
        "sed -i'' -e '36i\\\n#ifdef _LIBCPP_VERSION\\\n' include/SFML/System/String.hpp",
        "sed -i'' -e '37i\\\n#pragma clang diagnostic push\\\n' include/SFML/System/String.hpp",
        "sed -i'' -e '38i\\\n#pragma clang diagnostic ignored \"-Wdeprecated-declarations\"\\\n' include/SFML/System/String.hpp",
        "sed -i'' -e '39i\\\n#endif\\\n' include/SFML/System/String.hpp",
        "sed -i'' -e '619i\\\n#ifdef _LIBCPP_VERSION\\\n' include/SFML/System/String.hpp",
        "sed -i'' -e '620i\\\n#pragma clang diagnostic pop\\\n' include/SFML/System/String.hpp",
        "sed -i'' -e '621i\\\n#endif\\\n' include/SFML/System/String.hpp",
    ],
    strip_prefix = "SFML-2.6.1",
    url = "https://github.com/SFML/SFML/archive/2.6.1.tar.gz",
)

# https://github.com/gabime/spdlog
http_archive(
    name = "spdlog",  # MIT
    build_file = "//third_party:spdlog.BUILD",
    integrity = "sha256-FYZQgCmn0GcN/LLZdXXc3CQtOGiiWXQrafEAgBq04Ws=",
    strip_prefix = "spdlog-1.14.1",
    url = "https://github.com/gabime/spdlog/archive/v1.14.1.tar.gz",
)

# https://github.com/nothings/stb
http_archive(
    name = "stb",  # MIT/Unlicense
    build_file = "//third_party:stb.BUILD",
    integrity = "sha256-sBqpPhqWiu1V9D4HLJjuQB0vIOiXqr2xoWbHFmiG7RE=",
    strip_prefix = "stb-013ac3beddff3dbffafd5177e7972067cd2b5083",
    url = "https://github.com/nothings/stb/archive/013ac3beddff3dbffafd5177e7972067cd2b5083.tar.gz",
)

# https://www.unicode.org/Public/
http_archive(
    name = "ucd",
    build_file_content = """exports_files(["UnicodeData.txt"])""",
    integrity = "sha256-yxxmPQU5JlAM1QEilzYEV1JxOgZr11gCCYWYt6cFYXc=",
    url = "https://www.unicode.org/Public/15.1.0/ucd/UCD.zip",
)

# https://github.com/illiliti/libudev-zero
http_archive(
    name = "udev-zero",  # ISC
    build_file = "//third_party:udev-zero.BUILD",
    sha256 = "0bd89b657d62d019598e6c7ed726ff8fed80e8ba092a83b484d66afb80b77da5",
    strip_prefix = "libudev-zero-1.0.3",
    url = "https://github.com/illiliti/libudev-zero/archive/1.0.3.tar.gz",
)

VULKAN_TAG = "1.3.289"

# https://github.com/KhronosGroup/Vulkan-Headers
http_archive(
    name = "vulkan",  # Apache-2.0
    build_file = "//third_party:vulkan.BUILD",
    integrity = "sha256-/GbbbeZqbjUnwRD/Hbd6hq6XgE/n8BlyXo0lrNyHXG8=",
    strip_prefix = "Vulkan-Headers-%s" % VULKAN_TAG,
    url = "https://github.com/KhronosGroup/Vulkan-Headers/archive/v%s.tar.gz" % VULKAN_TAG,
)

# https://github.com/KhronosGroup/Vulkan-Hpp
http_archive(
    name = "vulkan_hpp",  # Apache-2.0
    build_file = "//third_party:vulkan_hpp.BUILD",
    integrity = "sha256-6RkF/LPOQ9EQouVMn4Wk1mV7ebUVr5ozwip4jNsmF14=",
    strip_prefix = "Vulkan-Hpp-%s" % VULKAN_TAG,
    url = "https://github.com/KhronosGroup/Vulkan-Hpp/archive/v%s.tar.gz" % VULKAN_TAG,
)

# HEAD as of 2024-07-13
# https://github.com/web-platform-tests/wpt
http_archive(
    name = "wpt",  # BSD-3-Clause
    build_file_content = """exports_files(["url/resources/urltestdata.json"])""",
    integrity = "sha256-fGBDYcOvggvyutrTNyDdW1Vgc5jTLtlbTWIS3WRSOiY=",
    strip_prefix = "wpt-f3dd9cba239a9655951ee62ec4dafc8fe37df2c5",
    url = "https://github.com/web-platform-tests/wpt/archive/f3dd9cba239a9655951ee62ec4dafc8fe37df2c5.tar.gz",
)

# The freedesktop GitLab goes down too often to be trusted.
# https://gitlab.freedesktop.org/xorg/lib/libxcursor
http_archive(
    name = "xcursor",  # MIT
    build_file = "//third_party:xcursor.BUILD",
    integrity = "sha256-m3DxifDxfAHudBII/B6zXbFqAb/u9iR7+XBUF5m0yVo=",
    strip_prefix = "libxcursor-libXcursor-1.2.2",
    urls = [
        "https://gitlab.freedesktop.org/xorg/lib/libxcursor/-/archive/libXcursor-1.2.2/libxcursor-libXcursor-1.2.2.tar.gz",
        # TODO(robinlinden): Mirror.
    ],
)

# https://gitlab.freedesktop.org/xorg/lib/libxext
http_archive(
    name = "xext",  # MIT
    build_file = "//third_party:xext.BUILD",
    integrity = "sha256-TkjqJxtfU8M4YBim4CY0VP5YKkE/zgJzreYB+/6eDHI=",
    strip_prefix = "libxext-libXext-1.3.6",
    urls = [
        "https://gitlab.freedesktop.org/xorg/lib/libxext/-/archive/libXext-1.3.6/libxext-libXext-1.3.6.tar.gz",
        "https://github.com/gitlab-freedesktop-mirrors/libxext/archive/refs/tags/libXext-1.3.6.tar.gz",
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

# https://github.com/madler/zlib
http_archive(
    name = "zlib",  # Zlib
    build_file = "//third_party:zlib.BUILD",
    integrity = "sha256-F+iIY/NgBnKrSRgvIXKBtvxNPHYr3jYZNeQ2qVIU0Fw=",
    strip_prefix = "zlib-1.3.1",
    url = "https://github.com/madler/zlib/archive/v1.3.1.tar.gz",
)

http_archive(
    name = "zstd",  # BSD-3-Clause
    build_file = "//third_party:zstd.BUILD",
    integrity = "sha256-jCngbPQqrMHq/EB3ri7Gxvy5amJhV+BZPV6Co0/UA8E=",
    strip_prefix = "zstd-1.5.6",
    url = "https://github.com/facebook/zstd/releases/download/v1.5.6/zstd-1.5.6.tar.gz",
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
    python_version = "3.12.3",
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
