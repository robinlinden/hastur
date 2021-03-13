#include "etest/etest.h"

using etest::require;

int main() {
    etest::test("this should fail", [] {
        require(false);
        std::exit(1); // Exit w/ failure if this line runs.
    });

    // Invert to return success on failure.
    return !etest::run_all_tests();
}
