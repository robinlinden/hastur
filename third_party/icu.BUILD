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
            "-I source/common/",
            "-I source/common/unicode/",
            "-I source/stubdata/",
        ],
        "//conditions:default": [
            "-frtti",
            "-I source/common/",
            "-I source/common/unicode/",
            "-I source/stubdata/",
            "-Wno-deprecated-declarations",
        ],
    }) + select({
        "@bazel_tools//tools/cpp:clang-cl": [
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
        "@platforms//os:windows": [],
        "//conditions:default": [
            "U_ELF",
        ],
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
    cmd = "./$(location gensprep) --destdir $(RULEDIR) --bundle-name " + input.replace(".txt", "").rpartition("/")[2] + " --norm-correction external/icu/source/data/unidata/ --unicode 15.0.0 $<",
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

genrule(
    name = "run_pkgdata",
    srcs = [
        "pkgdata.lst",
        "uts46.nrm",
    ] + SPREP_DATA_COMPILED,
    outs = ["libicudt73l.a"],
    cmd = r"""srcs=($(SRCS)); export PATH=$$PATH:$(location icupkg); $(location pkgdata) --entrypoint icudt73 --sourcedir $(RULEDIR) --destdir $(RULEDIR) --name icudt73l --mode static $${srcs[0]}""",
    tools = [
        ":icupkg",
        ":pkgdata",
    ],
    visibility = ["//visibility:public"],
)

genrule(
    name = "run_pkgdata_windows",
    srcs = [
        "pkgdata.lst",
        "uts46.nrm",
    ] + SPREP_DATA_COMPILED,
    outs = ["sicudt73l.lib"],
    cmd = r"""srcs=($(SRCS)); export PATH=$$PATH:$(location icupkg):"/$$('C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe' -latest -prerelease -find '**\lib.exe' | grep x64 | grep -v llvm | head -n1 | awk -F '\' 'BEGIN{OFS=FS} {$$NF=""; print}' | tr -d ':' | tr '\134' '/')"; $(location pkgdata) --entrypoint icudt73 --sourcedir $(RULEDIR) --destdir $(RULEDIR) --name icudt73l --mode static $${srcs[0]}""",
    tools = [
        ":icupkg",
        ":pkgdata",
    ],
    visibility = ["//visibility:public"],
)
