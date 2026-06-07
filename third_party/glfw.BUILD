load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:objc_library.bzl", "objc_library")

COMMON_SRCS = [
    "src/context.c",
    "src/egl_context.c",
    "src/init.c",
    "src/input.c",
    "src/internal.h",
    "src/mappings.h",
    "src/monitor.c",
    "src/null_init.c",
    "src/null_joystick.c",
    "src/null_joystick.h",
    "src/null_monitor.c",
    "src/null_platform.h",
    "src/null_window.c",
    "src/osmesa_context.c",
    "src/platform.c",
    "src/platform.h",
    "src/vulkan.c",
    "src/window.c",
    "src/xkb_unicode.c",
    "src/xkb_unicode.h",
]

objc_library(
    name = "glfw_macos",
    srcs = COMMON_SRCS + [
        "src/cocoa_joystick.h",
        "src/cocoa_platform.h",
        "src/cocoa_time.c",
        "src/cocoa_time.h",
        "src/posix_module.c",
        "src/posix_poll.c",
        "src/posix_thread.c",
        "src/posix_thread.h",
    ],
    hdrs = [
        "include/GLFW/glfw3.h",
        "include/GLFW/glfw3native.h",
    ],
    defines = ["_GLFW_COCOA"],
    includes = ["include/"],
    non_arc_srcs = [
        "src/cocoa_init.m",
        "src/cocoa_joystick.m",
        "src/cocoa_monitor.m",
        "src/cocoa_window.m",
        "src/nsgl_context.m",
    ],
    sdk_frameworks = [
        "Cocoa",
        "CoreFoundation",
        "IOKit",
        "OpenGL",
    ],
    target_compatible_with = ["@platforms//os:macos"],
)

cc_library(
    name = "glfw_linux",
    srcs = COMMON_SRCS + [
        "src/glx_context.c",
        "src/linux_joystick.c",
        "src/linux_joystick.h",
        "src/posix_module.c",
        "src/posix_poll.c",
        "src/posix_poll.h",
        "src/posix_thread.c",
        "src/posix_thread.h",
        "src/posix_time.c",
        "src/posix_time.h",
        "src/x11_init.c",
        "src/x11_monitor.c",
        "src/x11_platform.h",
        "src/x11_window.c",
    ],
    hdrs = [
        "include/GLFW/glfw3.h",
        "include/GLFW/glfw3native.h",
    ],
    # TODO(robinlinden): Needed until we've ported Xinerama to Bazel.
    features = ["-layering_check"],
    linkopts = [
        "-lX11",
        "-lXinerama",
    ],
    local_defines = ["_GLFW_X11"],
    strip_include_prefix = "include/",
    target_compatible_with = ["@platforms//os:linux"],
    deps = [
        "@xcursor",
        "@xext",
        "@xrandr",
    ],
)

cc_library(
    name = "glfw_windows",
    srcs = COMMON_SRCS + [
        "src/wgl_context.c",
        "src/win32_init.c",
        "src/win32_joystick.c",
        "src/win32_joystick.h",
        "src/win32_module.c",
        "src/win32_monitor.c",
        "src/win32_platform.h",
        "src/win32_thread.c",
        "src/win32_thread.h",
        "src/win32_time.c",
        "src/win32_time.h",
        "src/win32_window.c",
    ],
    hdrs = [
        "include/GLFW/glfw3.h",
        "include/GLFW/glfw3native.h",
    ],
    linkopts = [
        "-DEFAULTLIB:gdi32.lib",
        "-DEFAULTLIB:shell32.lib",
        "-DEFAULTLIB:user32.lib",
    ],
    local_defines = ["_GLFW_WIN32"],
    strip_include_prefix = "include/",
    target_compatible_with = ["@platforms//os:windows"],
)

alias(
    name = "glfw",
    actual = select({
        "@platforms//os:linux": ":glfw_linux",
        "@platforms//os:macos": ":glfw_macos",
        "@platforms//os:windows": ":glfw_windows",
    }),
    visibility = ["//visibility:public"],
)

# Example nonsense.

cc_library(
    name = "glad",
    hdrs = ["deps/glad/gl.h"],
    strip_include_prefix = "deps/",
)

cc_library(
    name = "linmath",
    hdrs = ["deps/linmath.h"],
    strip_include_prefix = "deps/",
)

cc_binary(
    name = "boing_example",
    srcs = ["examples/boing.c"],
    deps = [
        ":glad",
        ":glfw",
        ":linmath",
    ],
)
