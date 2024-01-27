load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_python//python:defs.bzl", "py_binary")

GENERATED_SRCS = [
    "src/bigreq.c",
    "src/xc_misc.c",
    "src/xproto.c",
]

GENERATED_HDRS = [
    "src/bigreq.h",
    "src/xc_misc.h",
    "src/xproto.h",
]

cc_library(
    name = "xcb",
    srcs = glob(["src/*.c"]) + GENERATED_SRCS,
    hdrs = glob(["src/*.h"]) + GENERATED_HDRS,
    defines = [
        "XCB_QUEUE_BUFFER_SIZE=16384",
        "IOV_MAX=16",
    ],
    strip_include_prefix = "src/",
    visibility = ["//visibility:public"],
    deps = [
        # ":generated",
        "@xau",
        "@xproto",
    ],
    # linkstatic = False,
)

py_binary(
    name = "c_client",
    srcs = ["src/c_client.py"],
    visibility = ["//visibility:public"],
    deps = [
        "@xcbproto//:xcbgen",
    ],
)

# cc_library(
#     name = "generated",
#     srcs = [
#         "bigreq.c",
#         "xc_misc.c",
#         "xproto.c",
#     ],
#     hdrs = [
#         "bigreq.h",
#         "xc_misc.h",
#         "xproto.h",
#     ],
# )

genrule(
    name = "xproto",
    srcs = ["@xcbproto//:xproto"],
    outs = [
        "src/xproto.h",
        "src/xproto.c",
    ],
    cmd = "$(location :c_client) -l dummy3 -c dummy2 -s dummy1 $< && mv xproto.* $(RULEDIR)/src/",
    tools = [":c_client"],
)

genrule(
    name = "bigreq",
    srcs = ["@xcbproto//:bigreq"],
    outs = [
        "src/bigreq.h",
        "src/bigreq.c",
    ],
    cmd = "$(location :c_client) -l dummy3 -c dummy2 -s dummy1 $< && mv bigreq.* $(RULEDIR)/src/",
    tools = [":c_client"],
)

genrule(
    name = "xc_misc",
    srcs = ["@xcbproto//:xc_misc"],
    outs = [
        "src/xc_misc.h",
        "src/xc_misc.c",
    ],
    cmd = "$(location :c_client) -l dummy3 -c dummy2 -s dummy1 $< && mv xc_misc.* $(RULEDIR)/src/",
    tools = [":c_client"],
)
