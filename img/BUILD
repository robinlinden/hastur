load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")
load("@rules_fuzzing//fuzzing:cc_defs.bzl", "cc_fuzz_test")
load("//bzl:copts.bzl", "HASTUR_COPTS", "HASTUR_FUZZ_PLATFORMS")

cc_library(
    name = "gif",
    srcs = ["gif.cpp"],
    hdrs = ["gif.h"],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
)

cc_library(
    name = "jpeg",
    srcs = ["jpeg.cpp"],
    hdrs = ["jpeg.h"],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
)

cc_library(
    name = "jpeg_turbo",
    srcs = ["jpeg_turbo.cpp"],
    hdrs = ["jpeg_turbo.h"],
    copts = HASTUR_COPTS,
    target_compatible_with = select({
        # libjpeg-turbo requires us to use setjmp/longjmp for error handling,
        # and our wasm setup doesn't get along with that.
        "@platforms//os:wasi": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    visibility = ["//visibility:public"],
    deps = ["@libjpeg-turbo"],
)

cc_library(
    name = "png",
    srcs = ["png.cpp"],
    hdrs = ["png.h"],
    copts = HASTUR_COPTS,
    implementation_deps = ["@libpng"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "qoi",
    srcs = ["qoi.cpp"],
    hdrs = ["qoi.h"],
    copts = HASTUR_COPTS,
    visibility = ["//visibility:public"],
    deps = ["@expected"],
)

# See: https://www.mjt.me.uk/posts/smallest-png/
genrule(
    name = "tiny_png",
    srcs = ["tiny.png"],
    outs = ["tiny_png.h"],
    cmd = "xxd -i $< >$@",
)

# See: https://web.archive.org/web/20111224041840/http://www.techsupportteam.org/forum/digital-imaging-photography/1892-worlds-smallest-valid-jpeg.html
genrule(
    name = "tiny_jpg",
    srcs = ["tiny.jpg"],
    outs = ["tiny_jpg.h"],
    cmd = "xxd -i $< >$@",
)

extra_srcs = {
    "jpeg_turbo": [":tiny_jpg"],
    "png": [":tiny_png"],
}

[cc_test(
    name = src.removesuffix(".cpp"),
    size = "small",
    srcs = [src] + extra_srcs.get(
        src.removesuffix("_test.cpp"),
        [],
    ),
    copts = HASTUR_COPTS,
    deps = [":%s" % src.removesuffix("_test.cpp")] + [
        "//etest",
        "@expected",
    ],
) for src in glob(
    include = ["*_test.cpp"],
    exclude = ["*_fuzz_test.cpp"],
)]

[cc_fuzz_test(
    name = src.removesuffix(".cpp"),
    size = "small",
    testonly = True,
    srcs = [src],
    copts = HASTUR_COPTS,
    target_compatible_with = HASTUR_FUZZ_PLATFORMS,
    deps = [":%s" % src.removesuffix("_fuzz_test.cpp")],
) for src in glob(["*_fuzz_test.cpp"])]

cc_binary(
    name = "img_example",
    srcs = ["img_example.cpp"],
    copts = HASTUR_COPTS,
    tags = ["no-cross"],
    deps = [
        ":gif",
        ":jpeg_turbo",
        ":png",
        ":qoi",
        "//gfx",
        "//gfx:sfml",
        "//type:sfml",
        "@sfml//:graphics",
        "@sfml//:window",
    ],
)
