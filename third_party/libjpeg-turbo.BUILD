load("@bazel_skylib//rules:expand_template.bzl", "expand_template")
load("@rules_cc//cc:defs.bzl", "cc_library")

# https://github.com/libjpeg-turbo/libjpeg-turbo/blob/36ac5b847047b27b90b459f5d44154773880196f/CMakeLists.txt#L605-L630
JPEG16_SOURCES = [
    "src/jcapistd.c",
    "src/jccolor.c",
    "src/jcdiffct.c",
    "src/jclossls.c",
    "src/jcmainct.c",
    "src/jcprepct.c",
    "src/jcsample.c",
    "src/jdapistd.c",
    "src/jdcolor.c",
    "src/jddiffct.c",
    "src/jdlossls.c",
    "src/jdmainct.c",
    "src/jdpostct.c",
    "src/jdsample.c",
    "src/jutils.c",
]

JPEG12_SOURCES = JPEG16_SOURCES + [
    "src/jccoefct.c",
    "src/jcdctmgr.c",
    "src/jdcoefct.c",
    "src/jddctmgr.c",
    "src/jdmerge.c",
    "src/jfdctfst.c",
    "src/jfdctint.c",
    "src/jidctflt.c",
    "src/jidctfst.c",
    "src/jidctint.c",
    "src/jidctred.c",
    "src/jquant1.c",
    "src/jquant2.c",
]

JPEG_SOURCES = JPEG12_SOURCES + [
    "src/jaricom.c",
    "src/jcapimin.c",
    "src/jcarith.c",
    "src/jchuff.c",
    "src/jcicc.c",
    "src/jcinit.c",
    "src/jclhuff.c",
    "src/jcmarker.c",
    "src/jcmaster.c",
    "src/jcomapi.c",
    "src/jcparam.c",
    "src/jcphuff.c",
    "src/jctrans.c",
    "src/jdapimin.c",
    "src/jdarith.c",
    "src/jdatadst.c",
    "src/jdatasrc.c",
    "src/jdhuff.c",
    "src/jdicc.c",
    "src/jdinput.c",
    "src/jdlhuff.c",
    "src/jdmarker.c",
    "src/jdmaster.c",
    "src/jdphuff.c",
    "src/jdtrans.c",
    "src/jerror.c",
    "src/jfdctflt.c",
    "src/jmemmgr.c",
    "src/jmemnobs.c",
    "src/jpeg_nbits.c",
]

# https://github.com/libjpeg-turbo/libjpeg-turbo/blob/36ac5b847047b27b90b459f5d44154773880196f/CMakeLists.txt#L2066-L2070
JPEG_HDRS = [
    "src/jerror.h",
    "src/jmorecfg.h",
    "src/jpeglib.h",
    ":generate_jconfig",
]

JPEG_C_INCLUDES = [
    "src/jccolext.c",
    "src/jdmerge.c",
    "src/jdmrg565.c",
    "src/jstdhuff.c",
    "src/jdcolext.c",
    "src/jdcol565.c",
    "src/jdmrgext.c",
]

JPEG_INTERNAL_HDRS = glob(
    include = ["src/*.h"],
    exclude = JPEG_HDRS,
) + [
    ":generate_jconfigint",
    ":generate_jversion",
    ":jconfig_hack",
]

expand_template(
    name = "generate_jconfig",
    out = "src/jconfig.h",
    substitutions = {
        "#cmakedefine C_ARITH_CODING_SUPPORTED 1": "#define C_ARITH_CODING_SUPPORTED 1",
        "#cmakedefine D_ARITH_CODING_SUPPORTED 1": "#define D_ARITH_CODING_SUPPORTED 1",
        "#cmakedefine RIGHT_SHIFT_IS_UNSIGNED 1": "// RIGHT_SHIFT_IS_UNSIGNED",
        "#cmakedefine WITH_SIMD 1": "// WITH_SIMD",
        "@JPEG_LIB_VERSION@": "62",
    },
    template = "src/jconfig.h.in",
)

genrule(
    name = "jconfig_hack",
    srcs = [":generate_jconfig"],
    outs = ["jconfig.h"],
    cmd = "cp $< $@",
)

expand_template(
    name = "generate_jconfigint",
    out = "jconfigint.h",
    substitutions = {
        "#cmakedefine C_ARITH_CODING_SUPPORTED 1": "#define C_ARITH_CODING_SUPPORTED 1",
        "#cmakedefine D_ARITH_CODING_SUPPORTED 1": "#define D_ARITH_CODING_SUPPORTED 1",
        "#cmakedefine HAVE_BUILTIN_CTZL": "// HAVE_BUILTIN_CTZL",
        "#cmakedefine HAVE_INTRIN_H": "// HAVE_INTRIN_H",
        "#cmakedefine WITH_SIMD 1": "// WITH_SIMD",
        "@CMAKE_PROJECT_NAME@": "libjpeg-turbo",
        "@HIDDEN@": "",
        "@INLINE@": "inline",
        "@SIZE_T@": "8",  # TODO(robinlinden): Support other values for sizeof(size_t).
        "@VERSION@": "3.1.0",
    },
    template = "src/jconfigint.h.in",
)

expand_template(
    name = "generate_jversion",
    out = "jversion.h",
    substitutions = {
        "@COPYRIGHT_YEAR@": "2024",
    },
    template = "src/jversion.h.in",
)

cc_library(
    name = "jpeg16",
    srcs = JPEG16_SOURCES + JPEG_INTERNAL_HDRS,
    hdrs = JPEG_HDRS,
    local_defines = ["BITS_IN_JSAMPLE=16"],
    textual_hdrs = JPEG_C_INCLUDES,
)

cc_library(
    name = "jpeg12",
    srcs = JPEG12_SOURCES + JPEG_INTERNAL_HDRS,
    hdrs = JPEG_HDRS,
    local_defines = ["BITS_IN_JSAMPLE=12"],
    textual_hdrs = JPEG_C_INCLUDES,
)

cc_library(
    name = "libjpeg-turbo",
    srcs = JPEG_SOURCES + JPEG_INTERNAL_HDRS,
    hdrs = JPEG_HDRS,
    implementation_deps = [
        ":jpeg12",
        ":jpeg16",
    ],
    strip_include_prefix = "src",
    textual_hdrs = JPEG_C_INCLUDES,
    visibility = ["//visibility:public"],
)
