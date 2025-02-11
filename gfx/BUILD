load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")
load("//bzl:copts.bzl", "HASTUR_COPTS")

cc_library(
    name = "gfx",
    srcs = ["color.cpp"],
    hdrs = [
        "canvas_command_saver.h",
        "color.h",
        "font.h",
        "icanvas.h",
    ],
    copts = HASTUR_COPTS,
    implementation_deps = ["//util:string"],
    visibility = ["//visibility:public"],
    deps = ["//geom"],
)

genrule(
    name = "basic_vertex_shader",
    srcs = ["basic_shader.vert"],
    outs = ["basic_vertex_shader.h"],
    cmd = "xxd -i $< >$@",
)

genrule(
    name = "rect_fragment_shader",
    srcs = ["rect_shader.frag"],
    outs = ["rect_fragment_shader.h"],
    cmd = "xxd -i $< >$@",
)

cc_library(
    name = "opengl",
    srcs = [
        "opengl_canvas.cpp",
        "opengl_shader.cpp",
        ":basic_vertex_shader",
        ":rect_fragment_shader",
    ],
    hdrs = [
        "opengl_canvas.h",
        "opengl_shader.h",
    ],
    copts = HASTUR_COPTS,
    implementation_deps = ["@glad2"],
    visibility = ["//visibility:public"],
    deps = [
        ":gfx",
        "//geom",
    ],
)

cc_library(
    name = "sfml",
    srcs = [
        "sfml_canvas.cpp",
        ":basic_vertex_shader",
        ":rect_fragment_shader",
    ],
    hdrs = ["sfml_canvas.h"],
    copts = HASTUR_COPTS + select({
        "@platforms//os:linux": [
            # sfml leaks this into our code.
            "-Wno-implicit-fallthrough",
        ],
        "@platforms//os:macos": [
            # sfml leaks this into our code.
            "-Wno-implicit-fallthrough",
        ],
        "//conditions:default": [],
    }),
    implementation_deps = [
        "//os:xdg",
        "@sfml//:system",
        "@spdlog",
    ],
    tags = ["no-cross"],
    visibility = ["//visibility:public"],
    deps = [
        ":gfx",
        "//geom",
        "//type:sfml",
        "@sfml//:graphics",
    ],
)

[cc_test(
    name = src.removesuffix(".cpp"),
    size = "small",
    srcs = [src],
    copts = HASTUR_COPTS,
    deps = [
        ":gfx",
        "//etest",
    ],
) for src in glob(["*_test.cpp"])]

cc_binary(
    name = "gfx_example",
    srcs = ["gfx_example.cpp"],
    copts = HASTUR_COPTS,
    tags = ["no-cross"],
    deps = [
        ":gfx",
        ":opengl",
        ":sfml",
        "//type:sfml",
        "@sfml//:graphics",
        "@sfml//:window",
    ],
)
