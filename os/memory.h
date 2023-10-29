// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef OS_MEMORY_H_
#define OS_MEMORY_H_

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>

namespace os {

class ExecutableMemory {
public:
    static std::optional<ExecutableMemory> allocate_containing(std::span<std::uint8_t const>);
    ~ExecutableMemory();

    ExecutableMemory(ExecutableMemory const &) = delete;
    ExecutableMemory &operator=(ExecutableMemory const &) = delete;

    ExecutableMemory(ExecutableMemory &&other) noexcept {
        std::swap(memory_, other.memory_);
        std::swap(size_, other.size_);
    }

    ExecutableMemory &operator=(ExecutableMemory &&other) noexcept {
        std::swap(memory_, other.memory_);
        std::swap(size_, other.size_);
        return *this;
    }

    void *ptr() { return memory_; }

private:
    ExecutableMemory(void *memory, std::size_t size) : memory_{memory}, size_{size} {}
    void *memory_{nullptr};
    std::size_t size_{};
};

} // namespace os

#endif
