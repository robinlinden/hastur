// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "os/memory.h"

#include <sys/mman.h>

#include <cstdlib>
#include <cstring>

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

    auto *memory = mmap(nullptr, data.size(), PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED) {
        return std::nullopt;
    }

    std::memcpy(memory, data.data(), data.size());

    if (mprotect(memory, data.size(), PROT_EXEC) != 0) {
        if (munmap(memory, data.size()) != 0) {
            std::abort();
        }

        return std::nullopt;
    }

    return ExecutableMemory{memory, data.size()};
}

} // namespace os
