// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "dom2/character_data.h"

#include "etest/etest.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

using namespace std::literals;
using etest::expect_eq;

using namespace dom2;

namespace {

class TestableCharacterData : public CharacterData {
public:
    TestableCharacterData(std::string data = ""s) : CharacterData(std::move(data)) {}

    // Not correct, but let's pretend this is fine so I can test the rest of the methods.
    NodeType type() const override { return NodeType::Text; }
};

} // namespace

int main() {
    etest::test("construction", [] {
        TestableCharacterData data{};
        expect_eq(data.data(), ""sv);
        expect_eq(data.length(), static_cast<std::size_t>(0));

        auto ohno = "oh no"s;
        data = TestableCharacterData{ohno};
        expect_eq(data.data(), ohno);
        expect_eq(data.length(), ohno.length());
    });

    etest::test("substring_data", [] {
        TestableCharacterData data{};
        expect_eq(data.substring_data(0, 0), ""sv);
        expect_eq(data.substring_data(0, 10), ""sv);

        data = TestableCharacterData{"oh no"s};
        expect_eq(data.substring_data(0, 100), "oh no"sv);
        expect_eq(data.substring_data(1, 100), "h no"sv);
        expect_eq(data.substring_data(1, 3), "h n"sv);
    });

    etest::test("append_data", [] {
        TestableCharacterData data{};
        data.append_data("test"sv);
        expect_eq(data.data(), "test"sv);
        data.append_data("y test"sv);
        expect_eq(data.data(), "testy test"sv);
    });

    etest::test("insert_data", [] {
        TestableCharacterData data{};
        data.insert_data(0, "test"sv);
        expect_eq(data.data(), "test"sv);
        data.insert_data(4, "ed"sv);
        data.insert_data(0, "very "sv);
        expect_eq(data.data(), "very tested"sv);
    });

    etest::test("delete_data", [] {
        TestableCharacterData data{"hello world"s};
        data.delete_data(5, 100);
        expect_eq(data.data(), "hello"sv);
        data.delete_data(0, 1);
        expect_eq(data.data(), "ello"sv);
    });

    etest::test("replace_data", [] {
        TestableCharacterData data{};
        data.replace_data(0, 0, "hello"sv);
        expect_eq(data.data(), "hello"sv);
        data.replace_data(4, 1, ""sv);
        expect_eq(data.data(), "hell"sv);
    });

    return etest::run_all_tests();
}
