// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef AZM_AMD64_ASSEMBLER_H_
#define AZM_AMD64_ASSEMBLER_H_

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace azm::amd64 {

enum class Reg32 : std::uint8_t {
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

// TODO(robinlinden): Clang (both w/ libc++ and libstdc++) doesn't like constexpr std::variant before clang-19.
struct Label {
    struct Linked {
        std::size_t offset{};
    };
    struct Unlinked {
        std::vector<std::size_t> patch_offsets{};
    };

    static Label linked(std::size_t jmp_target_offset) { return {Linked{jmp_target_offset}}; }
    static Label unlinked() { return {Unlinked{}}; }

    std::variant<Linked, Unlinked> v;
};

// https://www.felixcloutier.com/x86/
class Assembler {
public:
    [[nodiscard]] constexpr std::vector<std::uint8_t> take_assembled() { return std::exchange(assembled_, {}); }

    Label label() const { return Label::linked(assembled_.size()); }
    Label unlinked_label() const { return Label::unlinked(); }

    void link(Label &label) {
        assert(std::holds_alternative<Label::Unlinked>(label.v));
        constexpr int kInstructionSize = 4;
        std::size_t const jmp_target_offset = assembled_.size();

        auto const &unlinked = std::get<Label::Unlinked>(label.v);
        for (std::size_t patch_offset : unlinked.patch_offsets) {
            auto const rel32 = static_cast<std::uint32_t>(jmp_target_offset - patch_offset - kInstructionSize);
            assembled_[patch_offset + 0] = rel32 & 0xff;
            assembled_[patch_offset + 1] = (rel32 >> 8) & 0xff;
            assembled_[patch_offset + 2] = (rel32 >> 16) & 0xff;
            assembled_[patch_offset + 3] = (rel32 >> 24) & 0xff;
        }

        label = Label::linked(jmp_target_offset);
    }

    // Instructions
    constexpr void add(Reg32 dst, Imm32 imm32) {
        if (dst == Reg32::Eax) {
            emit(0x05);
            emit(imm32);
            return;
        }

        emit(0x81);
        mod_rm(0b11, 0, register_index(dst).value());
        emit(imm32);
    }

    void jmp(Label &label) {
        // JMP rel32
        if (std::holds_alternative<Label::Linked>(label.v)) {
            auto const &linked = std::get<Label::Linked>(label.v);
            auto const jmp_dst = static_cast<std::ptrdiff_t>(linked.offset - assembled_.size());
            constexpr int kShortInstructionSize = 2;
            if (jmp_dst >= (-128 + kShortInstructionSize) && jmp_dst <= 0) {
                emit(0xeb);
                emit(static_cast<std::uint8_t>(jmp_dst) - kShortInstructionSize);
                return;
            }

            constexpr int kNearInstructionSize = 5;
            emit(0xe9);
            emit(Imm32{static_cast<std::uint32_t>(jmp_dst - kNearInstructionSize)});
            return;
        }

        auto &unlinked = std::get<Label::Unlinked>(label.v);
        emit(0xe9);
        unlinked.patch_offsets.push_back(assembled_.size());
        emit(Imm32{0xdeadbeef});
    }

    constexpr void mov(Reg32 dst, Imm32 imm32) {
        auto idx = register_index(dst);
        assert(idx.has_value());
        emit(0xb8 + idx.value());
        emit(imm32);
    }

    constexpr void ret() { emit(0xc3); }

    constexpr void ud2() {
        emit(0x0f);
        emit(0x0b);
    }

private:
    constexpr void emit(std::uint8_t byte) { assembled_.push_back(byte); }
    constexpr void emit(Imm32 imm32) {
        for (auto i = 0; i < 4; ++i) {
            emit(imm32.v & 0xff);
            imm32.v >>= 8;
        }
    }

    constexpr void mod_rm(std::uint8_t mod, std::uint8_t reg, std::uint8_t rm) {
        assert(mod < 4);
        assert(reg < 8);
        assert(rm < 8);
        emit((mod << 6) | (reg << 3) | rm);
    }

    std::vector<std::uint8_t> assembled_;
};

} // namespace azm::amd64

#endif
