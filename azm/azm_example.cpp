// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "azm/assembler.h"

#include <algorithm>
#include <iostream>
#include <iterator>

int main() {
    using namespace azm;
    Amd64Assembler assembler;
    assembler.mov(Reg32::Eax, Imm32{3});
    assembler.add(Reg32::Eax, Imm32{39});
    assembler.mov(Reg32::Ecx, Imm32{0x4321});
    assembler.mov(Reg32::Edx, Imm32{0x12345678});
    assembler.mov(Reg32::Ebx, Imm32{0x1234});
    assembler.ret();
    assembler.ud2();
    auto code = assembler.take_assembled();
    // Print the machine code in a format usable for something like
    // `objdump -D -b binary -mi386:x86-64 -Mintel <file>`.
    std::ranges::copy(code, std::ostreambuf_iterator<char>(std::cout));
}
