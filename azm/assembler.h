// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef AZM_ASSEMBLER_H_
#define AZM_ASSEMBLER_H_

#include <cstdint>
#include <iostream>
#include <optional>
#include <utility>
#include <vector>

namespace azm {

enum class Reg32 {
    Eax,
    Ecx,
    Edx,
    Ebx,
};

struct Imm32 {
    std::uint32_t v{};
};

constexpr std::optional<std::uint8_t> register_index(Reg32 reg) {
    switch (reg) {
        case Reg32::Eax:
            return std::uint8_t{0};
        case Reg32::Ecx:
            return std::uint8_t{1};
        case Reg32::Edx:
            return std::uint8_t{2};
        case Reg32::Ebx:
            return std::uint8_t{3};
    }
    return std::nullopt;
}

// https://www.felixcloutier.com/x86/
class Amd64Assembler {
public:
    [[nodiscard]] std::vector<std::uint8_t> take_assembled() { return std::exchange(assembled_, {}); }

    void add(Reg32 dst, Imm32 imm32) {
        if (dst != Reg32::Eax) {
            std::cerr << "add: Unhandled dst " << static_cast<int>(dst) << '\n';
            ud2();
            return;
        }

        emit(0x05);
        emit(imm32);
    }

    void mov(Reg32 dst, Imm32 imm32) {
        emit(0xb8 + register_index(dst).value());
        emit(imm32);
    }

    void ret() { emit(0xc3); }

    void ud2() {
        emit(0x0f);
        emit(0x0b);
    }

private:
    void emit(std::uint8_t byte) { assembled_.push_back(byte); }
    void emit(Imm32 imm32) {
        for (auto i = 0; i < 4; ++i) {
            emit(imm32.v & 0xff);
            imm32.v >>= 8;
        }
    }

    std::vector<std::uint8_t> assembled_;
};

} // namespace azm

#endif
