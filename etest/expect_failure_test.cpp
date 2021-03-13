#include "etest/etest.h"

using etest::expect;

int main() {
    etest::test("this should fail", [] {
        expect(false);
    });

    // Invert to return success on failure.
    return !etest::run_all_tests();
}
