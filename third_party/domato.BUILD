load("@rules_python//python:defs.bzl", "py_binary")

py_binary(
    name = "domato",
    srcs = glob(["*.py"]),
    data = [
        "template.html",
    ] + glob(["rules/*.txt"]),
    imports = ["."],
    main = "generator.py",
    visibility = ["//visibility:public"],
)
