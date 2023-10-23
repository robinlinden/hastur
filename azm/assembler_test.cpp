// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "azm/assembler.h"

#include "etest/etest2.h"

#include <cstdint>
#include <type_traits>
#include <vector>

using CodeVec = std::vector<std::uint8_t>;

int main() {
    etest::Suite s{"assembler::amd64"};
    using namespace azm;

    s.add_test("Register index", [](etest::IActions &a) {
        a.expect_eq(register_index(Reg32::Eax), 0);
        a.expect_eq(register_index(Reg32::Ecx), 1);
        a.expect_eq(register_index(Reg32::Edx), 2);
        a.expect_eq(register_index(Reg32::Ebx), 3);
        a.expect_eq(register_index(static_cast<Reg32>(std::underlying_type_t<Reg32>{30})), std::nullopt);
    });

    s.add_test("MOV r32, imm32", [](etest::IActions &a) {
        Amd64Assembler assembler;

        assembler.mov(Reg32::Eax, Imm32{0xdeadbeef});
        a.expect_eq(assembler.take_assembled(), CodeVec{0xb8, 0xef, 0xbe, 0xad, 0xde});

        assembler.mov(Reg32::Edx, Imm32{0x1234});
        a.expect_eq(assembler.take_assembled(), CodeVec{0xba, 0x34, 0x12, 0, 0});
    });

    s.add_test("RET", [](etest::IActions &a) {
        Amd64Assembler assembler;

        assembler.ret();
        a.expect_eq(assembler.take_assembled(), CodeVec{0xc3});
    });

    return s.run();
}
