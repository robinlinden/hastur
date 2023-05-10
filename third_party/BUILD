test_suite(
    name = "third_party_tests",
    tests = [
        "@freetype2//:bbox_test",
        "@libpng//:pngtest",
        "@unifex//:unifex_tests",
    ],
)

# The drivers used on GH Actions don't work w/ vulkan,
# "ErrorIncompatibleDriver", but we still want to check that this builds.
alias(
    name = "vulkan_dynamic_loader_test",
    actual = "@vulkan_hpp//:dispatch_loader_dynamic_test",
)
