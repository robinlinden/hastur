// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "azm/amd64/assembler.h"

#include "etest/etest2.h"

#include <cstdint>
#include <optional>
#include <type_traits>
#include <vector>

using CodeVec = std::vector<std::uint8_t>;

int main() {
    etest::Suite s{"assembler::amd64"};
    using namespace azm::amd64;

    s.add_test("Register index", [](etest::IActions &a) {
        a.expect_eq(register_index(Reg32::Eax), 0);
        a.expect_eq(register_index(Reg32::Ecx), 1);
        a.expect_eq(register_index(Reg32::Edx), 2);
        a.expect_eq(register_index(Reg32::Ebx), 3);
        a.expect_eq(register_index(static_cast<Reg32>(std::underlying_type_t<Reg32>{30})), std::nullopt);
    });

    s.add_test("ADD reg32, imm32", [](etest::IActions &a) {
        Assembler assembler;

        // ADD EAX,imm32 generates slightly shorter asm than ADD w/ other registers.
        assembler.add(Reg32::Eax, Imm32{0x42});
        a.expect_eq(assembler.take_assembled(), CodeVec{0x05, 0x42, 0, 0, 0});

        // More general mod_rm-encoding for these registers.
        assembler.add(Reg32::Ecx, Imm32{0x42});
        a.expect_eq(assembler.take_assembled(), CodeVec{0x81, 0xc1, 0x42, 0, 0, 0});
        assembler.add(Reg32::Edx, Imm32{0x42});
        a.expect_eq(assembler.take_assembled(), CodeVec{0x81, 0xc2, 0x42, 0, 0, 0});
        assembler.add(Reg32::Ebx, Imm32{0x42});
        a.expect_eq(assembler.take_assembled(), CodeVec{0x81, 0xc3, 0x42, 0, 0, 0});
    });

    s.add_test("JMP, backwards", [](etest::IActions &a) {
        Assembler assembler;

        auto slot1 = assembler.label();
        assembler.jmp(slot1);
        assembler.ud2();
        assembler.jmp(slot1);
        auto slot2 = assembler.label();
        assembler.jmp(slot2);

        a.expect_eq(assembler.take_assembled(),
                CodeVec{
                        0xe9, // jmp rel32
                        0xfb, // -5
                        0xff,
                        0xff,
                        0xff,
                        0x0f, // ud2
                        0x0b,
                        0xe9, // jmp rel32
                        0xf4, // -12
                        0xff,
                        0xff,
                        0xff,
                        0xe9, // jmp rel32
                        0xfb, // -5
                        0xff,
                        0xff,
                        0xff,
                });
    });

    s.add_test("JMP, forwards", [](etest::IActions &a) {
        Assembler assembler;

        auto slot1 = assembler.unlinked_label();
        assembler.jmp(slot1);
        assembler.ud2();
        assembler.jmp(slot1);
        assembler.link(slot1);
        assembler.jmp(slot1);

        a.expect_eq(assembler.take_assembled(),
                CodeVec{
                        0xe9, // jmp rel32
                        0x07, // 7
                        0x00,
                        0x00,
                        0x00,
                        0x0f, // ud2
                        0x0b,
                        0xe9, // jmp rel32
                        0x00, // 0
                        0x00,
                        0x00,
                        0x00,
                        0xe9, // jmp rel32
                        0xfb, // -5
                        0xff,
                        0xff,
                        0xff,
                });
    });

    s.add_test("MOV r32, imm32", [](etest::IActions &a) {
        Assembler assembler;

        assembler.mov(Reg32::Eax, Imm32{0xdeadbeef});
        a.expect_eq(assembler.take_assembled(), CodeVec{0xb8, 0xef, 0xbe, 0xad, 0xde});

        assembler.mov(Reg32::Edx, Imm32{0x1234});
        a.expect_eq(assembler.take_assembled(), CodeVec{0xba, 0x34, 0x12, 0, 0});
    });

    s.add_test("RET", [](etest::IActions &a) {
        Assembler assembler;

        assembler.ret();
        a.expect_eq(assembler.take_assembled(), CodeVec{0xc3});
    });

    s.add_test("UD2", [](etest::IActions &a) {
        Assembler assembler;

        assembler.ud2();
        a.expect_eq(assembler.take_assembled(), CodeVec{0x0f, 0x0b});
    });

    return s.run();
}
