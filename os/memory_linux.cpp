// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "os/memory.h"

#include <sys/mman.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <span>

// Starting w/ LLVM 17, ubsan will read 8 bytes before the start of functions,
// so we have to add a few hacks to allocate extra memory and offset pointers to
// work around this undocumented(?) change.
//
// See:
// * https://github.com/llvm/llvm-project/issues/65253
// * https://github.com/llvm/llvm-project/commit/ad31a2dcadfcd57a99bbd6d0050d2690fd84a883
#ifndef __has_feature
#define __has_feature(x) 0
#endif
#if __has_feature(undefined_behavior_sanitizer)
static constexpr auto kUbsanOverreadWorkaroundSize = 8;
#else
static constexpr auto kUbsanOverreadWorkaroundSize = 0;
#endif

// Very randomly selected.
static constexpr auto kScrubByte = 0x2A;

namespace os {

ExecutableMemory::~ExecutableMemory() {
    if (memory_ != nullptr && munmap(memory_, size_) != 0) {
        std::abort();
    }
}

std::optional<ExecutableMemory> ExecutableMemory::allocate_containing(std::span<std::uint8_t const> data) {
    if (data.empty()) {
        return std::nullopt;
    }

    std::size_t actual_size = data.size() + kUbsanOverreadWorkaroundSize;

    auto *memory = static_cast<char *>(mmap(nullptr, actual_size, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    if (memory == MAP_FAILED) {
        return std::nullopt;
    }

    // NOLINTNEXTLINE(bugprone-suspicious-memset-usage): Size will be 0 when not using ubsan.
    std::memset(memory, kScrubByte, kUbsanOverreadWorkaroundSize);
    std::memcpy(memory + kUbsanOverreadWorkaroundSize, data.data(), data.size());

    if (mprotect(memory, data.size(), PROT_EXEC) != 0) {
        if (munmap(memory, data.size()) != 0) {
            std::abort();
        }

        return std::nullopt;
    }

    return ExecutableMemory{memory, data.size()};
}

void *ExecutableMemory::ptr() {
    return static_cast<char *>(memory_) + kUbsanOverreadWorkaroundSize;
}

} // namespace os
