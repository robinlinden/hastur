load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "x11",
    srcs = [":ks_tables"] + glob(
        include = [
            "src/*.c",
            "src/*.h",
            "src/xcms/*.c",
            "src/xcms/*.h",
            "src/xkb/*.c",
            "src/xkb/*.h",
            "src/xlibi18n/*.c",
            "src/xlibi18n/*.h",
            "src/xlibi18n/lcUniConv/*.h",
            "modules/im/ximcp/*.c",
            "modules/lc/Utf8/lcUTF8Load.c",
            "modules/lc/def/lcDefConv.c",
            "modules/lc/gen/lcGenConv.c",
            "modules/om/generic/*.c",
        ],
        exclude = [
            "src/os2Stubs.c",
            "modules/im/ximcp/imTrans.c",
        ],
    ),
    hdrs = glob(["include/X11/**/*.h"]),
    copts = [
        "-Iexternal/x11/src/",
        "-Iexternal/x11/src/xcms/",
        "-Iexternal/x11/src/xkb/",
        "-Iexternal/x11/src/xlibi18n/",
        "-Iexternal/x11/include/X11/",
    ],
    # linkstatic = False,
    local_defines = [
        "HAVE_SYS_IOCTL_H",
        "XKB",
        'XCMSDIR=\\"/usr/lib/X11\\"',
        'XLOCALELIBDIR=\\"/usr/lib/locale\\"',
    ],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
    deps = ["@xcb"],
)

cc_binary(
    name = "makekeys",
    srcs = ["src/util/makekeys.c"],
)

genrule(
    name = "ks_tables",
    srcs = ["@xproto//:keysymdef"],
    outs = ["ks_tables.h"],
    cmd = "$(location :makekeys) $< > $@",
    tools = [":makekeys"],
)
