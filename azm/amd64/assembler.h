// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef AZM_AMD64_ASSEMBLER_H_
#define AZM_AMD64_ASSEMBLER_H_

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <utility>
#include <vector>

namespace azm::amd64 {

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

struct Label {
    std::size_t offset{};
};

struct UnlinkedLabel {
    std::vector<std::size_t> patch_offsets{};
};

// https://www.felixcloutier.com/x86/
class Assembler {
public:
    [[nodiscard]] std::vector<std::uint8_t> take_assembled() { return std::exchange(assembled_, {}); }

    Label label() const { return Label{assembled_.size()}; }
    UnlinkedLabel unlinked_label() const { return UnlinkedLabel{}; }

    Label link(UnlinkedLabel const &label) {
        static constexpr int kInstructionSize = 4;
        std::size_t const jmp_target_offset = assembled_.size();

        for (std::size_t patch_offset : label.patch_offsets) {
            auto const rel32 = static_cast<std::uint32_t>(jmp_target_offset - patch_offset - kInstructionSize);
            assembled_[patch_offset + 0] = rel32 & 0xff;
            assembled_[patch_offset + 1] = (rel32 >> 8) & 0xff;
            assembled_[patch_offset + 2] = (rel32 >> 16) & 0xff;
            assembled_[patch_offset + 3] = (rel32 >> 24) & 0xff;
        }

        return Label{jmp_target_offset};
    }

    // Instructions
    void add(Reg32 dst, Imm32 imm32) {
        if (dst != Reg32::Eax) {
            std::cerr << "add: Unhandled dst " << static_cast<int>(dst) << '\n';
            ud2();
            return;
        }

        emit(0x05);
        emit(imm32);
    }

    void jmp(Label label) {
        // JMP rel32
        emit(0xe9);
        static constexpr int kInstructionSize = 4;
        emit(Imm32{static_cast<std::uint32_t>(label.offset - assembled_.size() - kInstructionSize)});
    }

    void jmp(UnlinkedLabel &label) {
        // JMP rel32
        emit(0xe9);
        label.patch_offsets.push_back(assembled_.size());
        emit(Imm32{0xdeadbeef});
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

} // namespace azm::amd64

#endif
