load("@rules_cc//cc:defs.bzl", "cc_library")

genrule(
    name = "xcursor_hdrs",
    srcs = ["include/X11/Xcursor/Xcursor.h.in"],
    outs = ["X11/Xcursor/Xcursor.h"],
    cmd = "cp $< $@",
)

genrule(
    name = "internal_xcursor_hdrs",
    srcs = [":xcursor_hdrs"],
    outs = ["Xcursor.h"],
    cmd = "cp $< $@",
)

cc_library(
    name = "xcursor",
    srcs = [":internal_xcursor_hdrs"] + glob([
        "src/*.c",
        "src/*.h",
    ]),
    hdrs = [":xcursor_hdrs"],
    visibility = ["//visibility:public"],
    deps = ["@xrender"],
)
