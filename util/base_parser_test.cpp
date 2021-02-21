#include "util/base_parser.h"

#include <catch2/catch.hpp>

using util::BaseParser;

namespace {

template<bool B>
bool static_test() {
  static_assert(B);
  return B;
}

TEST_CASE("base_parser", "[base_parser]") {
    constexpr auto abcd = BaseParser("abcd");

    SECTION("peek") {
        REQUIRE(static_test<abcd.peek() == 'a'>());
        REQUIRE(static_test<abcd.peek(2) == "ab">());
        REQUIRE(static_test<abcd.peek(3) == "abc">());
        REQUIRE(static_test<abcd.peek(4) == "abcd">());
        REQUIRE(static_test<BaseParser(" ").peek() == ' '>());
    }

    SECTION("starts_with") {
        REQUIRE(static_test<!abcd.starts_with("hello")>());
        REQUIRE(static_test<abcd.starts_with("ab")>());
        REQUIRE(static_test<abcd.starts_with("abcd")>());
    }

    SECTION("is_eof, advance") {
        REQUIRE(static_test<!abcd.is_eof()>());
        REQUIRE(static_test<BaseParser("").is_eof()>());
        REQUIRE(static_test<[abcd] {
            auto p = abcd;
            p.advance(3);
            if (p.is_eof()) { return false; }
            p.advance(1);
            return p.is_eof();
        }()>());
    }

    SECTION("consume_char") {
        REQUIRE(static_test<[abcd] {
            auto p = abcd;
            if (p.consume_char() != 'a') { return false; }
            if (p.consume_char() != 'b') { return false; }
            if (p.consume_char() != 'c') { return false; }
            if (p.consume_char() != 'd') { return false; }
            return true;
        }()>());
    }

    SECTION("consume_while") {
        REQUIRE(static_test<[abcd] {
            auto p = abcd;
            if (p.consume_while([](char c) { return c != 'c'; }) != "ab") { return false; }
            if (p.consume_while([](char c) { return c != 'd'; }) != "c") { return false; }
            return true;
        }()>());
    }

    SECTION("skip_whitespace, consume_char") {
        REQUIRE(static_test<[] {
            auto p = BaseParser("      \t       \n         h          \n\n\ni");
            p.skip_whitespace();
            if (p.consume_char() != 'h') { return false; }
            p.skip_whitespace();
            if (p.consume_char() != 'i') { return false; }
            return true;
        }()>());
    }
}

} // namespace
