// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/variant.h"

#include "etest/etest.h"

#include <array>
#include <span>
#include <variant>

using etest::expect;

namespace {

// This is to help turn arrays into spans.
template<typename... Ts>
constexpr bool matches(auto const &buffer) {
    return util::Sequence<Ts...>::match(std::span{buffer});
}

} // namespace

int main() {
    etest::test("simple", [] {
        using Token = std::variant<int, unsigned>;
        expect(matches<int>(std::array{Token{1}}));
        expect(matches<unsigned>(std::array{Token{1u}}));

        expect(!matches<int>(std::array{Token{1u}}));
        expect(!matches<unsigned>(std::array{Token{1}}));

        expect(matches<unsigned, unsigned>(std::to_array<Token>({1u, 1u})));
        expect(matches<int, int>(std::to_array<Token>({1, 1})));
        expect(matches<int, unsigned>(std::to_array<Token>({1, 1u})));
        expect(matches<unsigned, int>(std::to_array<Token>({1u, 1})));

        expect(!matches<unsigned>(std::to_array<Token>({1u, 1u})));
        expect(!matches<int>(std::to_array<Token>({1, 1})));
        expect(!matches<int, int, int>(std::to_array<Token>({1, 1})));
        expect(!matches<int, int>(std::to_array<Token>({1u, 1u})));
        expect(!matches<unsigned, int>(std::to_array<Token>({1, 1})));
        expect(!matches<unsigned, unsigned>(std::to_array<Token>({1, 1})));
    });

    etest::test("parser-ish", [] {
        struct LParen {};
        struct RParen {};
        struct Comma {};
        struct IntLiteral {
            int v{};
        };

        using Token = std::variant<LParen, RParen, Comma, IntLiteral>;

        expect(matches<LParen, IntLiteral, RParen>( //
                std::to_array<Token>({LParen{}, IntLiteral{13}, RParen{}})));

        expect(matches<LParen, IntLiteral, Comma, IntLiteral, RParen>( //
                std::to_array<Token>({LParen{}, IntLiteral{13}, Comma{}, IntLiteral{5}, RParen{}})));

        expect(!matches<LParen, LParen>(std::to_array<Token>({LParen{}, RParen{}})));
    });

    return etest::run_all_tests();
}
