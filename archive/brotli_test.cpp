// SPDX-FileCopyrightText: 2024 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "archive/brotli.h"

#include "etest/etest2.h"

#include <tl/expected.hpp>

int main() {
    etest::Suite s{"brotli"};

    using namespace archive;

    s.add_test("trivial decode",
            [](etest::IActions &a) { a.expect_eq(brotli_decode({}), tl::unexpected{BrotliError::InputEmpty}); });

    return s.run();
}
