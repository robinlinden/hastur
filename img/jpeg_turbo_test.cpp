// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "img/jpeg_turbo.h"

#include "etest/etest2.h"

#include <tuple>

int main() {
    etest::Suite s;

    s.add_test("it can run", [](etest::IActions &) {
        std::ignore = img::JpegTurbo::from({}); //
    });

    return s.run();
}
