// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "azm/amd64/assembler.h"

#include <algorithm>
#include <iostream>
#include <iterator>

int main() {
    using namespace azm::amd64;
    Assembler assembler;
    auto forward = assembler.unlinked_label();
    assembler.jmp(forward);
    assembler.mov(Reg32::Eax, Imm32{3});
    assembler.add(Reg32::Eax, Imm32{39});
    assembler.mov(Reg32::Ecx, Imm32{0x4321});

    auto end = assembler.label();
    assembler.ret();

    assembler.link(forward);
    assembler.mov(Reg32::Edx, Imm32{0x12345678});
    assembler.mov(Reg32::Ebx, Imm32{0x1234});

    assembler.add(Reg32::Ecx, Imm32{0x1234});
    assembler.add(Reg32::Edx, Imm32{0x1234'5678});
    assembler.add(Reg32::Ebx, Imm32{0x5678});

    assembler.jmp(end);

    assembler.ud2();

    auto code = assembler.take_assembled();
    // Print the machine code in a format usable for something like
    // `objdump -D -b binary -mi386:x86-64 -Mintel <file>`.
    std::ranges::copy(code, std::ostreambuf_iterator<char>(std::cout));
}
