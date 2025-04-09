// SPDX-FileCopyrightText: 2023 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2024-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "url/percent_encode.h"

#include "etest/etest2.h"

#include <string>

int main() {
    etest::Suite s;

    s.add_test("uppercase percent-encoded triplets", [](etest::IActions &a) {
        using url::percent_encoded_triplets_to_upper;
        std::string foo{"https://example.com/%ff"};
        std::string foo2{"%be%ee%ee%ff"};
        std::string foo3;
        std::string foo4{"%"};
        std::string foo5{"%77"};
        std::string foo6{"%EE"};

        a.expect_eq(percent_encoded_triplets_to_upper(foo), "https://example.com/%FF");
        a.expect_eq(percent_encoded_triplets_to_upper(foo2), "%BE%EE%EE%FF");
        a.expect_eq(percent_encoded_triplets_to_upper(foo3), "");
        a.expect_eq(percent_encoded_triplets_to_upper(foo4), "%");
        a.expect_eq(percent_encoded_triplets_to_upper(foo5), "%77");
        a.expect_eq(percent_encoded_triplets_to_upper(foo6), "%EE");
    });

    s.add_test("percent-decode URL unreserved", [](etest::IActions &a) {
        using url::percent_decode_unreserved;
        std::string foo{"https://example.com/%7e"};
        std::string foo2{"%7e%30%61%2D%2e%5F"};
        std::string foo3;
        std::string foo4{"%"};
        std::string foo5{"%77"};
        std::string foo6{"%7F"};

        a.expect_eq(percent_decode_unreserved(foo), "https://example.com/~");
        a.expect_eq(percent_decode_unreserved(foo2), "~0a-._");
        a.expect_eq(percent_decode_unreserved(foo3), "");
        a.expect_eq(percent_decode_unreserved(foo4), "%");
        a.expect_eq(percent_decode_unreserved(foo5), "w");
        a.expect_eq(percent_decode_unreserved(foo6), "%7F");
    });

    return s.run();
}
