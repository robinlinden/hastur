load("@rules_python//python:pip.bzl", "compile_pip_requirements")

# //third_party:requirements_test ocasionally times out on slower machines (like
# when running on GH Actions), so let's give it a generous timeout.
compile_pip_requirements(
    name = "requirements",
    timeout = "moderate",
    requirements_in = "requirements.in",
    requirements_txt = "requirements.txt",
    tags = [
        "no-cross",
        # This also creates a "requirements_test" which calls into bash which is
        # very leaky and upsets valgrind.
        "no-valgrind",
    ],
)

test_suite(
    name = "third_party_tests",
    tags = ["no-clang-tidy"],
    tests = [
        "@freetype2//:bbox_test",
        "@libpng//:pngtest",
    ],
)

# The drivers used on GH Actions don't work w/ vulkan,
# "ErrorIncompatibleDriver", but we still want to check that this builds.
alias(
    name = "vulkan_dynamic_loader_test",
    actual = "@vulkan_hpp//:dispatch_loader_dynamic_test",
    tags = ["no-clang-tidy"],
)
