// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "layout/box_model.h"

#include "etest/etest.h"

using etest::expect;

int main() {
    etest::test("BoxModel box models", [] {
        layout::BoxModel box{
                .content{.x = 400, .y = 400, .width = 100, .height = 100}, // x: 400-500, y: 400-500
                .padding{.left = 100, .right = 100, .top = 100, .bottom = 100}, // x: 300-600, y: 300-600
                .border{.left = 100, .right = 100, .top = 100, .bottom = 100}, // x: 200-700, y: 200-700
                .margin{.left = 100, .right = 100, .top = 100, .bottom = 100}, // x: 100-800, y: 100-800
        };

        expect(box.padding_box() == geom::Rect{300, 300, 300, 300});
        expect(box.border_box() == geom::Rect{200, 200, 500, 500});
        expect(box.margin_box() == geom::Rect{100, 100, 700, 700});
    });

    etest::test("BoxModel::contains", [] {
        layout::BoxModel box{
                .content{.x = 400, .y = 400, .width = 100, .height = 100}, // x: 400-500, y: 400-500
                .padding{.left = 100, .right = 100, .top = 100, .bottom = 100}, // x: 300-600, y: 300-600
                .border{.left = 100, .right = 100, .top = 100, .bottom = 100}, // x: 200-700, y: 200-700
                .margin{.left = 100, .right = 100, .top = 100, .bottom = 100}, // x: 100-800, y: 100-800
        };

        expect(box.contains({450, 450})); // Inside content.
        expect(box.contains({300, 300})); // Inside padding.
        expect(box.contains({650, 250})); // Inside border.
        expect(!box.contains({150, 150})); // Inside margin.
        expect(!box.contains({90, 90})); // Outside margin.
    });

    return etest::run_all_tests();
}
