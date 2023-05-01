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
    visibility = ["//visibility:public"],
    deps = [
        "//geom",
        "//util:string",
    ],
)

cc_library(
    name = "opengl",
    srcs = ["opengl_canvas.cpp"],
    hdrs = ["opengl_canvas.h"],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":gfx",
        "@glew",
    ],
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
        "//conditions:default": [],
    }),
    visibility = ["//visibility:public"],
    deps = [
        ":gfx",
        "//os",
        "//util:string",
        "@sfml//:graphics",
        "@spdlog",
    ],
)

[cc_test(
    name = src[:-4],
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
    deps = [
        ":gfx",
        ":opengl",
        ":sfml",
        "@sfml//:graphics",
        "@sfml//:window",
    ],
)
