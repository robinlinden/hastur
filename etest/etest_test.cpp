#include "etest/etest.h"

auto test0 = etest::test("expect true works", [] {
    etest::expect(true);
});
