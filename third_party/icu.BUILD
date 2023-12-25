load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_library(
    name = "common",
    srcs = glob([
        "source/common/*.h",
        "source/common/*.cpp",
        "source/stubdata/*.h",
        "source/stubdata/*.cpp",
    ]),
    hdrs = glob([
        "source/common/*.h",
        "source/common/unicode/*.h",
    ]),
    copts = select({
        "@platforms//os:windows": [
            "/GR",
            "-Isource/common/",
            "-Isource/common/unicode/",
            "-Isource/stubdata/",
        ],
        "//conditions:default": [
            "-frtti",
            "-I source/common/",
            "-I source/common/unicode/",
            "-I source/stubdata/",
            "-Wno-deprecated-declarations",
        ],
    }) + select({
        "@rules_cc//cc/compiler:clang-cl": [
            "-Wno-microsoft-include",
        ],
        "//conditions:default": [],
    }),
    defines = [
        "U_STATIC_IMPLEMENTATION",
        "U_CHARSET_IS_UTF8=1",
        "U_HIDE_OBSOLETE_UTF_OLD_H=1",
    ],
    linkopts = select({
        "@platforms//os:windows": [
            "-DEFAULTLIB:advapi32",
        ],
        "//conditions:default": ["-ldl"],
    }),
    linkstatic = True,
    local_defines = [
        "U_COMMON_IMPLEMENTATION",
    ],
    strip_include_prefix = "source/common/",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "toolutil",
    srcs = glob(["source/tools/toolutil/*.cpp"]),
    hdrs = glob(["source/tools/toolutil/*.h"]),
    copts = select({
        "@platforms//os:windows": [
            "/GR",
        ],
        "//conditions:default": [
            "-frtti",
        ],
    }),
    linkstatic = True,
    local_defines = ["U_TOOLUTIL_IMPLEMENTATION"] + select({
        "@platforms//os:linux": ["U_ELF"],
        "//conditions:default": [],
    }),
    strip_include_prefix = "source/tools/toolutil",
    visibility = ["//visibility:private"],
    deps = [
        ":common",
        ":i18n",
    ],
)

cc_library(
    name = "i18n",
    srcs = glob(["source/i18n/*.cpp"]),
    hdrs = glob([
        "source/i18n/*.h",
        "source/i18n/unicode/*.h",
    ]),
    copts = select({
        "@platforms//os:windows": [
            "/GR",
        ],
        "//conditions:default": [
            "-frtti",
        ],
    }),
    linkstatic = True,
    local_defines = [
        "U_I18N_IMPLEMENTATION",
    ],
    strip_include_prefix = "source/i18n",
    visibility = ["//visibility:private"],
    deps = [":common"],
)

cc_binary(
    name = "gensprep",
    srcs = glob(["source/tools/gensprep/*.c"]) + ["source/tools/gensprep/gensprep.h"],
    visibility = ["//visibility:private"],
    deps = [
        ":common",
        ":i18n",
        ":toolutil",
    ],
)

SPREP_DATA = glob(["source/data/sprep/*.txt"])

SPREP_DATA_COMPILED = [s.replace("txt", "spp").rpartition("/")[2] for s in SPREP_DATA]

filegroup(
    name = "normalizations",
    srcs = ["source/data/unidata/NormalizationCorrections.txt"],
)

[genrule(
    name = "run_sprep_" + input.replace(".txt", "").rpartition("/")[2],
    srcs = [input],
    outs = [input.replace("txt", "spp").rpartition("/")[2]],
    cmd = "./$(location gensprep) --destdir $(RULEDIR) --bundle-name " + input.replace(".txt", "").rpartition("/")[2] + " --norm-correction external/icu/source/data/unidata/ --unicode 15.1.0 $<",
    tools = [
        ":gensprep",
        ":normalizations",
    ],
    visibility = ["//visibility:private"],
) for input in SPREP_DATA]

genrule(
    name = "create_pkgdata_lst",
    srcs = SPREP_DATA_COMPILED,
    outs = ["pkgdata.lst"],
    cmd = "echo -e \"" + "\\n".join(SPREP_DATA_COMPILED) + "\" > $(RULEDIR)/pkgdata.lst && echo uts46.nrm >> $(RULEDIR)/pkgdata.lst",
)

genrule(
    name = "move uts46.nrm",
    srcs = ["source/data/in/uts46.nrm"],
    outs = ["uts46.nrm"],
    cmd = "cp $< $(RULEDIR)",
)

cc_binary(
    name = "icupkg",
    srcs = ["source/tools/icupkg/icupkg.cpp"],
    visibility = ["//visibility:private"],
    deps = [
        ":common",
        ":i18n",
        ":toolutil",
    ],
)

cc_binary(
    name = "pkgdata",
    srcs = [
        "source/tools/pkgdata/pkgdata.cpp",
        "source/tools/pkgdata/pkgtypes.c",
        "source/tools/pkgdata/pkgtypes.h",
    ],
    visibility = ["//visibility:private"],
    deps = [
        ":common",
        ":i18n",
        ":toolutil",
    ],
)

PKGDATA_INC_MACOS = r""" echo "
GENCCODE_ASSEMBLY_TYPE=-a gcc-darwin
SO=dylib
SOBJ=dylib
A=a
LIBPREFIX=lib
LIB_EXT_ORDER=.74.1.dylib
COMPILE=clang -DU_ATTRIBUTE_DEPRECATED=   -DU_HAVE_STRTOD_L=1 -DU_HAVE_XLOCALE_H=1 -DU_HAVE_STRING_VIEW=1  -O2 -std=c11 -Wall -pedantic -Wshadow -Wpointer-arith -Wmissing-prototypes -Wwrite-strings  -Qunused-arguments -Wno-parentheses-equality -fno-common -c
LIBFLAGS=-dynamic
GENLIB=clang -dynamiclib -dynamic -O2 -std=c11 -Wall -pedantic -Wshadow -Wpointer-arith -Wmissing-prototypes -Wwrite-strings  -Qunused-arguments -Wno-parentheses-equality
LDICUDTFLAGS=
LD_SONAME=-Wl,-compatibility_version -Wl,74 -Wl,-current_version -Wl,74.1 -install_name
RPATH_FLAGS=
BIR_LDFLAGS=
AR=ar
ARFLAGS=r -c
RANLIB=ranlib
INSTALL_CMD=install -c" > $(RULEDIR)/pkgdata.inc
"""

PKGDATA_INC = r""" echo "
GENCCODE_ASSEMBLY_TYPE=-a gcc
SO=so
SOBJ=so
A=a
LIBPREFIX=lib
LIB_EXT_ORDER=.74.1
COMPILE=gcc -D_REENTRANT  -DU_HAVE_ELF_H=1 -DU_HAVE_STRTOD_L=1 -DU_HAVE_XLOCALE_H=0  -DU_DISABLE_RENAMING=1 -DU_ATTRIBUTE_DEPRECATED= -march=native -O2 -pipe -std=c11 -Wall -pedantic -Wshadow -Wpointer-arith -Wmissing-prototypes -Wwrite-strings   -c
LIBFLAGS=-DPIC -fPIC
GENLIB=gcc -march=native -O2 -pipe -std=c11 -Wall -pedantic -Wshadow -Wpointer-arith -Wmissing-prototypes -Wwrite-strings   -Wl,-O1 -Wl,--as-needed  -shared -Wl,-Bsymbolic
LDICUDTFLAGS=
LD_SONAME=-Wl,-soname -Wl,
RPATH_FLAGS=
BIR_LDFLAGS=-Wl,-Bsymbolic
AR=ar
ARFLAGS=r
RANLIB=ranlib
INSTALL_CMD=install -c" > $(RULEDIR)/pkgdata.inc
"""

# https://github.com/unicode-org/icu/blob/main/icu4c/source/tools/pkgdata/pkgdata.cpp#L2206
# https://github.com/unicode-org/icu/blob/main/icu4c/source/tools/pkgdata/pkgdata.cpp#L179
# For generating the data lib, ICU uses build options from a "pkgdata.inc" file generated and installed as part of the normal ICU build. We don't do a "normal" ICU build, so we have to provide our own.
genrule(
    name = "pkgdata_inc",
    outs = ["pkgdata.inc"],
    cmd = select({
        "@platforms//os:macos": PKGDATA_INC_MACOS,
        "//conditions:default": PKGDATA_INC,
    }),
    visibility = ["//visibility:private"],
)

genrule(
    name = "run_pkgdata",
    srcs = [
        "pkgdata.lst",
        "uts46.nrm",
    ] + SPREP_DATA_COMPILED,
    outs = ["libicudt74l.a"],
    cmd = r"""
        srcs=($(SRCS));
        export PATH=$$PATH:$(location icupkg);
        $(location pkgdata) --bldopt $(location pkgdata_inc) --entrypoint icudt74 --sourcedir $(RULEDIR) --destdir $(RULEDIR) --name icudt74l --mode static $${srcs[0]}
    """,
    tools = [
        ":icupkg",
        ":pkgdata",
        ":pkgdata_inc",
    ],
    visibility = ["//visibility:public"],
)

genrule(
    name = "run_pkgdata_windows",
    srcs = [
        "pkgdata.lst",
        "uts46.nrm",
    ] + SPREP_DATA_COMPILED,
    outs = ["sicudt74l.lib"],
    cmd = r"""
        srcs=($(SRCS));
        export PATH=$$PATH:$(location icupkg):"/$$('C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe' -latest -prerelease -find 'VC\Tools\MSVC\*\bin\Hostx64\x64\lib.exe' | grep -v llvm | head -n1 | awk -F '\' 'BEGIN{OFS=FS} {$$NF=""; print}' | tr -d ':' | tr '\134' '/')";
        $(location pkgdata) --entrypoint icudt74 --sourcedir $(RULEDIR) --destdir $(RULEDIR) --name icudt74l --mode static $${srcs[0]}
    """,
    tools = [
        ":icupkg",
        ":pkgdata",
    ],
    visibility = ["//visibility:public"],
)
