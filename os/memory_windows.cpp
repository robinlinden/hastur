// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "os/memory.h"

#include "os/windows_setup.h" // IWYU pragma: keep

#include <Memoryapi.h>

#include <cstdlib>
#include <cstring>

// Kernel32
namespace os {

// VirtualFree has a weird 2nd argument:
// [in] dwSize - The size of the region of memory to be freed, in bytes.
// If the dwFreeType parameter is MEM_RELEASE, this parameter must be 0 (zero).
// https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualfree
ExecutableMemory::~ExecutableMemory() {
    if (memory_ != nullptr && (VirtualFree(memory_, 0, MEM_RELEASE) == 0)) {
        std::abort();
    }
}

std::optional<ExecutableMemory> ExecutableMemory::allocate_containing(std::span<std::uint8_t const> data) {
    if (data.empty()) {
        return std::nullopt;
    }

    auto *memory = VirtualAlloc(nullptr, data.size(), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (memory == nullptr) {
        return std::nullopt;
    }

    std::memcpy(memory, data.data(), data.size());

    DWORD old_protect{};
    if ((VirtualProtect(memory, data.size(), PAGE_EXECUTE, &old_protect) == 0)
            || (FlushInstructionCache(GetCurrentProcess(), memory, data.size()) == 0)) {
        if (VirtualFree(memory, 0, MEM_RELEASE) == 0) {
            std::abort();
        }

        return std::nullopt;
    }

    return ExecutableMemory{memory, data.size()};
}

void *ExecutableMemory::ptr() {
    return memory_;
}

} // namespace os
