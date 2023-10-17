load("@pypi//:requirements.bzl", "entry_point")
load("@rules_cc//cc:defs.bzl", "cc_library")

alias(
    name = "run_glad",
    actual = entry_point("glad2", "glad"),
)

# gl:core=3.1 is the last core profile that supports deprecated functions like
# glMatrixMode, glColor4ub, and glRecti that we still use.
# TODO(robinlinden): Modernize our GL setup.
GLAD_CMD = "$(location :run_glad) --reproducible --api 'gl:core=3.1' --out-path $(RULEDIR) c --loader ;"

GLAD_HDRS = [
    "KHR/khrplatform.h",
    "glad/gl.h",
]

GLAD_SRCS = ["src/gl.c"]

genrule(
    name = "generate_glad",
    outs = GLAD_HDRS + GLAD_SRCS,
    cmd = GLAD_CMD + "mv $(RULEDIR)/include/* $(RULEDIR)",
    tools = [":run_glad"],
)

cc_library(
    name = "glad",
    srcs = GLAD_SRCS,
    hdrs = GLAD_HDRS,
    includes = ["."],
    visibility = ["//visibility:public"],
)
