load("@rules_cc//cc:defs.bzl", "cc_library")

#GLIB_MAJOR_VERSION = 2
#GLIB_MINOR_VERSION = 67
#GLIB_MICRO_VERSION = 5
#
#GLIB_CONFIG = {
#    "GLIB_MAJOR_VERSION": GLIB_MAJOR_VERSION,
#    "GLIB_MINOR_VERSION": GLIB_MINOR_VERSION,
#    "GLIB_MICRO_VERSION": GLIB_MICRO_VERSION,
#    "GLIB_INTERFACE_AGE": 0 if (GLIB_MINOR_VERSION % 2 != 0) else GLIB_MICRO_VERSION,
#    "GLIB_BINARY_AGE": 100 * GLIB_MINOR_VERSION + GLIB_MICRO_VERSION,
#    "GETTEXT_PACKAGE": "glib20",
#    "ENABLE_NLS": -1,
#    "BROKEN_POLL": select({
#        "@platforms//os:windows": True,
#        "//conditions:default": False
#    })
#}
#
#GLIBCONFIG_CONFIG = {
#    "LT_CURRENT_MINUS_AGE": 0,
#    "G_HAVE_GNUC_VISIBILITY": select({
#        "@platforms//os:windows": False,
#        "//conditions:default": True
#    })
#}
#
#LIBRARY_VERSION = "{}.{}.{}".format(0, GLIB_CONFIG['GLIB_BINARY_AGE'] - GLIB_CONFIG['GLIB_INTERFACE_AGE'], GLIB_CONFIG['GLIB_INTERFACE_AGE'])

cc_library(
    name = "glib",
    srcs = select({
        "@platforms//os:linux": glob(
            ["glib/*.c", "glib/*.h"],
            exclude = [
                "glib/gstdio-private.c",
                "glib/gwin32.c",
                "glib/gwin32-private.c",
                "glib/gspawn-win32.c",
                "glib/giowin32.c",
                "glib/win_iconv.c",
                "glib/gthread-win32.c",
                "glib/gspawn-win32-helper.c",
            ],
        ),
        "@platforms//os:windows": glob(
            ["glib/*.c", "glib/*.h", "glib/dirent/wdirent.c"],
            exclude = [
                "glib/glib-unix.c",
                "glib/gspawn.c",
                "glib/giounix.c",
            ],
        ),
    }),
    copts = [
        "-Iexternal/glib",
        "-Iexternal/glib/glib",
        #"-Iexternal/glib/glib/deprecated",
        #"-Iexternal/glib/glib/libcharset",
        "-DGLIB_COMPILATION",
    ],
    hdrs = glob([
        "glib/**/*.h",
    ]),
    deps = ["@//third_party:glib_config"],
    visibility = ["//visibility:public"],
)

#cc_library(
#    name = "gobject",
#)
#cc_library(
#    name = "gthread",
#)
#cc_library(
#    name = "gmodule",
#)
#cc_library(
#    name = "gio",
#)
