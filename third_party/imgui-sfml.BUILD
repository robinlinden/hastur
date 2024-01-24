load("@rules_cc//cc:defs.bzl", "cc_library")

# TODO(robinlinden): False positive layering check error. Related to glad being
# a local_repository? See: bazel-out/k8-fastbuild/bin/external/glad/glad.cppmap
#
# external/imgui-sfml/imgui-SFML.cpp:4:10: error: module imgui-sfml//:imgui-sfml
# does not depend on a module exporting 'glad/gl.h'
package(features = ["-layering_check"])

cc_library(
    name = "imgui-sfml",
    srcs = ["imgui-SFML.cpp"],
    hdrs = glob(["*.h"]),
    defines = ["IMGUI_SFML_SHARED_LIB=0"],
    includes = ["."],
    visibility = ["//visibility:public"],
    deps = [
        "@glad",
        "@imgui",
        "@sfml//:graphics",
        "@sfml//:system",
        "@sfml//:window",
    ],
)
