#include "util/base_parser.h"

#include "etest/etest.h"

using etest::expect_true;
using util::BaseParser;

namespace {

template<bool B>
bool static_test() {
  static_assert(B);
  return B;
}

} // namespace

int main() {
    etest::test("peek", [] {
        constexpr auto abcd = BaseParser("abcd");
        expect_true(static_test<abcd.peek() == 'a'>());
        expect_true(static_test<abcd.peek(2) == "ab">());
        expect_true(static_test<abcd.peek(3) == "abc">());
        expect_true(static_test<abcd.peek(4) == "abcd">());
        expect_true(static_test<BaseParser(" ").peek() == ' '>());
    });

    etest::test("starts_with", [] {
        constexpr auto abcd = BaseParser("abcd");
        expect_true(static_test<!abcd.starts_with("hello")>());
        expect_true(static_test<abcd.starts_with("ab")>());
        expect_true(static_test<abcd.starts_with("abcd")>());
    });

#ifndef __clang__ // Clang doesn't yet support lambdas in templates.
    etest::test("is_eof, advance", [] {
        constexpr auto abcd = BaseParser("abcd");
        expect_true(static_test<!abcd.is_eof()>());
        expect_true(static_test<BaseParser("").is_eof()>());
        expect_true(static_test<[] {
            auto p = BaseParser("abcd");
            p.advance(3);
            if (p.is_eof()) { return false; }
            p.advance(1);
            return p.is_eof();
        }()>());
    });

    etest::test("consume_char", [] {
        expect_true(static_test<[] {
            auto p = BaseParser("abcd");
            if (p.consume_char() != 'a') { return false; }
            if (p.consume_char() != 'b') { return false; }
            if (p.consume_char() != 'c') { return false; }
            if (p.consume_char() != 'd') { return false; }
            return true;
        }()>());
    });

    etest::test("consume_while", [] {
        expect_true(static_test<[] {
            auto p = BaseParser("abcd");
            if (p.consume_while([](char c) { return c != 'c'; }) != "ab") { return false; }
            if (p.consume_while([](char c) { return c != 'd'; }) != "c") { return false; }
            return true;
        }()>());
    });

    etest::test("skip_whitespace, consume_char", [] {
        expect_true(static_test<[] {
            auto p = BaseParser("      \t       \n         h          \n\n\ni");
            p.skip_whitespace();
            if (p.consume_char() != 'h') { return false; }
            p.skip_whitespace();
            if (p.consume_char() != 'i') { return false; }
            return true;
        }()>());
    });
#endif
}
