load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")

cc_library(
    name = "gfx",
    hdrs = [
        "canvas_command_saver.h",
        "color.h",
        "font.h",
        "icanvas.h",
        "painter.h",
    ],
    visibility = ["//visibility:public"],
    deps = ["//geom"],
)

cc_library(
    name = "opengl",
    srcs = ["opengl_canvas.cpp"],
    hdrs = ["opengl_canvas.h"],
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
    visibility = ["//visibility:public"],
    deps = [
        ":gfx",
        "//os",
        "@sfml//:graphics",
        "@spdlog",
    ],
)

[cc_test(
    name = src[:-4],
    size = "small",
    srcs = [src],
    deps = [
        ":gfx",
        "//etest",
    ],
) for src in glob(["*_test.cpp"])]

cc_binary(
    name = "gfx_example",
    srcs = ["gfx_example.cpp"],
    deps = [
        ":gfx",
        ":opengl",
        ":sfml",
        "@sfml//:graphics",
        "@sfml//:window",
    ],
)
