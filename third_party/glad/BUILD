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

genrule(
    name = "generate_glad_hdrs",
    outs = [
        "glad/gl.h",
        "KHR/khrplatform.h",
    ],
    cmd = GLAD_CMD + """
        mv $(RULEDIR)/include/glad $(RULEDIR) ;
        mv $(RULEDIR)/include/KHR $(RULEDIR)
    """,
    tools = [":run_glad"],
)

genrule(
    name = "generate_glad_srcs",
    outs = ["gl.c"],
    cmd = GLAD_CMD + """
        mv $(RULEDIR)/src/gl.c $(RULEDIR)
    """,
    tools = [":run_glad"],
)

cc_library(
    name = "glad",
    srcs = [":generate_glad_srcs"],
    hdrs = [":generate_glad_hdrs"],
    includes = ["."],
    visibility = ["//visibility:public"],
)
