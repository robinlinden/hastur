load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive", "http_file")
load("@bazel_tools//tools/build_defs/repo:local.bzl", "local_repository")

# Bazel
# =========================================================

# https://github.com/bazelbuild/apple_support
http_archive(
    name = "build_bazel_apple_support",
    integrity = "sha256-tT9kkedCVJ8ThmYo3f/MddHzstaYfcTxShayQhE8iQs=",
    url = "https://github.com/bazelbuild/apple_support/releases/download/1.17.1/apple_support.1.17.1.tar.gz",
)

# https://github.com/bazelbuild/platforms
http_archive(
    name = "platforms",  # Apache-2.0
    integrity = "sha256-IY7+juc20mo1cmY7N0olPAErcW2K8MB+hC6C8jigp+4=",
    url = "https://github.com/bazelbuild/platforms/releases/download/0.0.10/platforms-0.0.10.tar.gz",
)

# https://github.com/bazelbuild/rules_cc
# 0.1.0 isn't compatible w/ Bazel 7.4.0:
# ERROR: Traceback (most recent call last):
#   File "<...>/external/remote_java_tools/BUILD", line 7, column 60, in <toplevel>
#     load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_proto_library")
# Error: file '@rules_cc//cc:defs.bzl' does not contain symbol 'cc_proto_library'
http_archive(
    name = "rules_cc",  # Apache-2.0
    integrity = "sha256-9Krdg4fzgQM6mtBQBEOlKgzqX4rR7eQ2nTxhTreyaC4=",
    patch_cmds = [
        # rules_cc depends on protobuf as of 0.0.13, and adding that for rules_cc is silly.
        # https://github.com/bazelbuild/rules_cc/commit/013a08285803532d9c5de010da51dd45b4cd2722
        "sed -i'' -e /@com_google_protobuf/d cc/defs.bzl",
        "sed -i'' -e 's/_cc_proto_library/native.cc_proto_library/g' cc/defs.bzl",
    ],
    strip_prefix = "rules_cc-0.0.15",
    url = "https://github.com/bazelbuild/rules_cc/releases/download/0.0.15/rules_cc-0.0.15.tar.gz",
)

# https://github.com/bazelbuild/rules_fuzzing
http_archive(
    name = "rules_fuzzing",
    integrity = "sha256-5rwhm/rJ4fg7Mn3QkPcoqflz7pm5tdjloYSicy7whiM=",
    strip_prefix = "rules_fuzzing-0.5.2",
    url = "https://github.com/bazelbuild/rules_fuzzing/releases/download/v0.5.2/rules_fuzzing-0.5.2.zip",
)

# https://github.com/bazelbuild/rules_license
http_archive(
    name = "rules_license",  # Apache-2.0
    sha256 = "26d4021f6898e23b82ef953078389dd49ac2b5618ac564ade4ef87cced147b38",
    url = "https://github.com/bazelbuild/rules_license/releases/download/1.0.0/rules_license-1.0.0.tar.gz",
)

# https://github.com/bazelbuild/rules_python
# TODO(robinlinden): Work out how to get glad2 working w/ newer rules_python.
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
    integrity = "sha256-kHdFv5FVX3foI0wLlTNx5srFunFdHPEv9kFJbdG86dE=",
    url = "https://github.com/uber/hermetic_cc_toolchain/releases/download/v3.1.1/hermetic_cc_toolchain-v3.1.1.tar.gz",
)

# Misc tools
# =========================================================

# HEAD as of 2024-10-28.
# https://github.com/hedronvision/bazel-compile-commands-extractor
http_archive(
    name = "hedron_compile_commands",
    integrity = "sha256-ZYEiz7HyW+duohKwD16wR9jirci8+SO5GEYfKx43zfI=",
    strip_prefix = "bazel-compile-commands-extractor-4f28899228fb3ad0126897876f147ca15026151e",
    url = "https://github.com/hedronvision/bazel-compile-commands-extractor/archive/4f28899228fb3ad0126897876f147ca15026151e.tar.gz",
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
    integrity = "sha256-8blLgO6wC7Y6PIzvUEfU5AnfTYo/5QIwWXaWWCfZVnI=",
    strip_prefix = "asio-asio-1-32-0",
    url = "https://github.com/chriskohlhoff/asio/archive/asio-1-32-0.tar.gz",
)

# HEAD as of 2024-11-07.
# https://github.com/google/boringssl
http_archive(
    name = "boringssl",  # OpenSSL + ISC
    integrity = "sha256-fsRzsD2NDeHXdOQe0YqpzHfA6aIZV8ShItGj5whZrB8=",
    strip_prefix = "boringssl-52a2c003d9622a78d6b791c10ea456eabaf6f52a",
    url = "https://github.com/google/boringssl/archive/52a2c003d9622a78d6b791c10ea456eabaf6f52a.tar.gz",
)

# https://github.com/google/brotli
http_archive(
    name = "brotli",  # MIT
    integrity = "sha256-5yCmyilCi4A/StFlNxdx9TmPq6OX7fZ3iDehhZnqE/8=",
    patch_cmds = ["""sed -i'' -e 's/package(/package(features=["-layering_check"],/' BUILD.bazel"""],
    strip_prefix = "brotli-1.1.0",
    url = "https://github.com/google/brotli/archive/refs/tags/v1.1.0.tar.gz",
)

# https://github.com/tartanllama/expected
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
    integrity = "sha256-bLHm03vct1bbvlm+Q4eQ20Cc20hoxm6IjV358T98An8=",
    strip_prefix = "fmt-11.0.2",
    url = "https://github.com/fmtlib/fmt/archive/11.0.2.tar.gz",
)

# https://github.com/freetype/freetype
http_archive(
    name = "freetype2",  # FTL
    build_file = "//third_party:freetype2.BUILD",
    integrity = "sha256-vFyJjkdW03Pg2ZG6sFMDbF6yqnwNXGfoZi3cbaQMQQM=",
    strip_prefix = "freetype-VER-2-13-3",
    url = "https://github.com/freetype/freetype/archive/VER-2-13-3.tar.gz",
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
    integrity = "sha256-osRDQE8ACY6ekKzyncMY4EnS3HjZrl9G77Jhk0pzDOI=",
    patch_cmds = [
        "rm source/common/BUILD.bazel",
        "rm source/stubdata/BUILD.bazel",
        "rm source/tools/toolutil/BUILD.bazel",
        "rm source/i18n/BUILD.bazel",

        # icu 76.1's https://github.com/unicode-org/icu/commit/66ba09973a4231711b6de0de042f4e532b1873e5
        # causes pkgdata to segfault due to faulty-looking platform detection.
        # WINDOWS_WITH_MSVC is defined by icu, meaning that
        # pkg_createOptMatchArch never sets the arch to anything, and the
        # nullptr arch is later used like strcmp(nullptr, "x64") in
        # getArchitecture, so let's hack out their fixes.
        "sed -i'' -e 's/if defined(__clang__)/if 0/' source/tools/toolutil/pkg_genc.cpp",
    ],
    strip_prefix = "icu-release-76-1/icu4c",
    url = "https://github.com/unicode-org/icu/archive/refs/tags/release-76-1.tar.gz",
)

# https://www.unicode.org/Public/idna/
http_file(
    name = "idna_mapping_table",
    sha256 = "402cbd285f1f952fcd0834b63541d54f69d3d8f1b8f8599bf71a1a14935f82c4",
    url = "https://www.unicode.org/Public/idna/15.1.0/IdnaMappingTable.txt",
)

# https://github.com/ocornut/imgui
# 1.91.5 isn't supported by any version of imgui-sfml yet.
http_archive(
    name = "imgui",  # MIT
    build_file = "//third_party:imgui.BUILD",
    integrity = "sha256-pFXCjZh8eN31aquYzg/w/aeRojouyIreRt0Qa4N/CSM=",
    strip_prefix = "imgui-1.91.4",
    url = "https://github.com/ocornut/imgui/archive/v1.91.4.tar.gz",
)

# https://github.com/SFML/imgui-sfml
# HEAD as of 2024-10-16.
http_archive(
    name = "imgui-sfml",  # MIT
    build_file = "//third_party:imgui-sfml.BUILD",
    integrity = "sha256-IQXHW+Dsnuc8+6RgdKfEMAZ1mfaMXOQj0SEcmZ2VldM=",
    patch_cmds = [
        # Use glad for OpenGL instead of the system OpenGL headers.
        "sed -i'' -e /OpenGL.hpp/d imgui-SFML.cpp",
        "sed -i'' -e '4i\\\n#include <glad/gl.h>\\\n' imgui-SFML.cpp",
        "sed -i'' -e '240i\\\n\\\tif (gladLoaderLoadGL() == 0) std::abort();\\\n' imgui-SFML.cpp",
    ],
    strip_prefix = "imgui-sfml-560baa9ae707bd6866ac91da04b514519234494a",
    url = "https://github.com/SFML/imgui-sfml/archive/560baa9ae707bd6866ac91da04b514519234494a.tar.gz",
)

# https://github.com/simdjson/simdjson
http_archive(
    name = "simdjson",  # Apache-2.0
    build_file = "//third_party:simdjson.BUILD",
    integrity = "sha256-Ho+IHLLA9ibFbNNmWDLx6XudT/xkitnhBnwTSGK7oGA=",
    strip_prefix = "simdjson-3.10.1",
    url = "https://github.com/simdjson/simdjson/archive/refs/tags/v3.10.1.tar.gz",
)

# https://github.com/glennrp/libpng
http_archive(
    name = "libpng",  # Libpng
    build_file = "//third_party:libpng.BUILD",
    integrity = "sha256-DvW2M9DGX3gMT87Sf/gymY5xR4wTtF37bpTyOoL2T3w=",
    strip_prefix = "libpng-1.6.44",
    url = "https://github.com/glennrp/libpng/archive/v1.6.44.tar.gz",
)

# https://github.com/SFML/SFML
http_archive(
    name = "sfml",  # Zlib
    build_file = "//third_party:sfml.BUILD",
    integrity = "sha256-aA2n9DmKV2Z4fqnKad0cmkhxzzxINN362SJCXG95Lcg=",
    patch_cmds = [
        # SFML uses a non-standard include path to vulkan.h
        # libvulkan-dev: /usr/include/vulkan/vulkan.h
        "sed -i'' -e 's|vulkan.h|vulkan/vulkan.h|' src/SFML/Window/Win32/VulkanImplWin32.cpp",
        "sed -i'' -e 's|vulkan.h|vulkan/vulkan.h|' src/SFML/Window/Unix/VulkanImplX11.cpp",
    ],
    strip_prefix = "SFML-3.0.0-rc.1",
    url = "https://github.com/SFML/SFML/archive/3.0.0-rc.1.tar.gz",
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
    integrity = "sha256-TcL/psbI2Kgw6S/NuX+JgXAcIpZV331g5vwP9Oa0v2Y=",
    strip_prefix = "stb-2e2bef463a5b53ddf8bb788e25da6b8506314c08",
    url = "https://github.com/nothings/stb/archive/2e2bef463a5b53ddf8bb788e25da6b8506314c08.tar.gz",
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

VULKAN_TAG = "1.3.301"

# https://github.com/KhronosGroup/Vulkan-Headers
http_archive(
    name = "vulkan",  # Apache-2.0
    build_file = "//third_party:vulkan.BUILD",
    integrity = "sha256-bAKUm+1/OYTh0SJjvc5SocmeVKGrza6Q0AUnwokMHMU=",
    strip_prefix = "Vulkan-Headers-%s" % VULKAN_TAG,
    url = "https://github.com/KhronosGroup/Vulkan-Headers/archive/v%s.tar.gz" % VULKAN_TAG,
)

# https://github.com/KhronosGroup/Vulkan-Hpp
http_archive(
    name = "vulkan_hpp",  # Apache-2.0
    build_file = "//third_party:vulkan_hpp.BUILD",
    integrity = "sha256-dHkZudRLdi40Y9/2kZ/1VK3Xv8Ss8e+9Vi/S+/KjPjA=",
    strip_prefix = "Vulkan-Hpp-%s" % VULKAN_TAG,
    url = "https://github.com/KhronosGroup/Vulkan-Hpp/archive/v%s.tar.gz" % VULKAN_TAG,
)

# Stuck on the last commit where the archive has the same content every
# download. git_repository sort of works if we want to upgrade, but it's so slow
# that downloading wpt occasionally times out in CI.
# See: https://github.com/web-platform-tests/wpt/issues/47124
# https://github.com/web-platform-tests/wpt
http_archive(
    name = "wpt",  # BSD-3-Clause
    build_file_content = """exports_files(["url/resources/urltestdata.json"])""",
    integrity = "sha256-sUgB+WnWZ3UEjMoPO5kL4g2kot0TigulBNHbCTi4v9A=",
    strip_prefix = "wpt-13861f4a19afa26daa9e2a4ca2dcce82fc2e1236",
    url = "https://github.com/web-platform-tests/wpt/archive/13861f4a19afa26daa9e2a4ca2dcce82fc2e1236.tar.gz",
)

# https://gitlab.freedesktop.org/xorg/lib/libxcursor
http_archive(
    name = "xcursor",  # MIT
    build_file = "//third_party:xcursor.BUILD",
    integrity = "sha256-m3DxifDxfAHudBII/B6zXbFqAb/u9iR7+XBUF5m0yVo=",
    strip_prefix = "libxcursor-libXcursor-1.2.2",
    url = "https://gitlab.freedesktop.org/xorg/lib/libxcursor/-/archive/libXcursor-1.2.2/libxcursor-libXcursor-1.2.2.tar.gz",
)

# https://gitlab.freedesktop.org/xorg/lib/libxext
http_archive(
    name = "xext",  # MIT
    build_file = "//third_party:xext.BUILD",
    integrity = "sha256-TkjqJxtfU8M4YBim4CY0VP5YKkE/zgJzreYB+/6eDHI=",
    strip_prefix = "libxext-libXext-1.3.6",
    url = "https://gitlab.freedesktop.org/xorg/lib/libxext/-/archive/libXext-1.3.6/libxext-libXext-1.3.6.tar.gz",
)

# https://gitlab.freedesktop.org/xorg/lib/libxrandr
http_archive(
    name = "xrandr",  # MIT
    build_file = "//third_party:xrandr.BUILD",
    sha256 = "a1909cbe9ded94187b6420ae8c347153f8278955265cb80a64cdae5501433396",
    strip_prefix = "libxrandr-libXrandr-1.5.4",
    url = "https://gitlab.freedesktop.org/xorg/lib/libxrandr/-/archive/libXrandr-1.5.4/libxrandr-libXrandr-1.5.4.tar.gz",
)

# https://gitlab.freedesktop.org/xorg/lib/libxrender
http_archive(
    name = "xrender",  # MIT
    build_file = "//third_party:xrender.BUILD",
    sha256 = "4cd5aca5b948a80bb7c3d5060eb97b8a8199234c0c19fe34d35c5c838923230b",
    strip_prefix = "libxrender-libXrender-0.9.11",
    url = "https://gitlab.freedesktop.org/xorg/lib/libxrender/-/archive/libXrender-0.9.11/libxrender-libXrender-0.9.11.tar.gz",
)

# https://github.com/madler/zlib
http_archive(
    name = "zlib",  # Zlib
    build_file = "//third_party:zlib.BUILD",
    integrity = "sha256-F+iIY/NgBnKrSRgvIXKBtvxNPHYr3jYZNeQ2qVIU0Fw=",
    strip_prefix = "zlib-1.3.1",
    url = "https://github.com/madler/zlib/archive/v1.3.1.tar.gz",
)

# https://github.com/facebook/zstd
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

load("@bazel_features//:deps.bzl", "bazel_features_deps")

bazel_features_deps()

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
