// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "util/variant.h"

#include "etest/etest2.h"

#include <array>
#include <span>
#include <variant>

namespace {

// This is to help turn arrays into spans.
template<typename... Ts>
constexpr bool matches(auto const &buffer) {
    return util::Sequence<Ts...>::match(std::span{buffer});
}

} // namespace

int main() {
    etest::Suite s;

    s.add_test("simple", [](etest::IActions &a) {
        using Token = std::variant<int, unsigned>;
        a.expect(matches<int>(std::array{Token{1}}));
        a.expect(matches<unsigned>(std::array{Token{1u}}));

        a.expect(!matches<int>(std::array{Token{1u}}));
        a.expect(!matches<unsigned>(std::array{Token{1}}));

        a.expect(matches<unsigned, unsigned>(std::to_array<Token>({1u, 1u})));
        a.expect(matches<int, int>(std::to_array<Token>({1, 1})));
        a.expect(matches<int, unsigned>(std::to_array<Token>({1, 1u})));
        a.expect(matches<unsigned, int>(std::to_array<Token>({1u, 1})));

        a.expect(!matches<unsigned>(std::to_array<Token>({1u, 1u})));
        a.expect(!matches<int>(std::to_array<Token>({1, 1})));
        a.expect(!matches<int, int, int>(std::to_array<Token>({1, 1})));
        a.expect(!matches<int, int>(std::to_array<Token>({1u, 1u})));
        a.expect(!matches<unsigned, int>(std::to_array<Token>({1, 1})));
        a.expect(!matches<unsigned, unsigned>(std::to_array<Token>({1, 1})));
    });

    s.add_test("parser-ish", [](etest::IActions &a) {
        struct LParen {};
        struct RParen {};
        struct Comma {};
        struct IntLiteral {
            int v{};
        };

        using Token = std::variant<LParen, RParen, Comma, IntLiteral>;

        a.expect(matches<LParen, IntLiteral, RParen>( //
                std::to_array<Token>({LParen{}, IntLiteral{13}, RParen{}})));

        a.expect(matches<LParen, IntLiteral, Comma, IntLiteral, RParen>( //
                std::to_array<Token>({LParen{}, IntLiteral{13}, Comma{}, IntLiteral{5}, RParen{}})));

        a.expect(!matches<LParen, LParen>(std::to_array<Token>({LParen{}, RParen{}})));
    });

    s.add_test("holds_any_of", [](etest::IActions &a) {
        struct Foo {};
        struct Bar {};
        struct Baz {};

        std::variant<Foo, Bar, Baz> var = Foo{};

        a.expect(util::holds_any_of<Foo, Bar, Baz>(var));
        a.expect(!util::holds_any_of<Bar, Baz>(var));
    });

    return s.run();
}
