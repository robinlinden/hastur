// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "os/memory.h"

#include "etest/etest2.h"

#include <cstdint>
#include <optional>

// __amd64 => GNU C, _M_AMD64 => MSVC.
#if defined(__amd64) || defined(_M_AMD64)
#define AMD64
#endif

int main() {
    etest::Suite s{"os/memory"};

    s.add_test("ExecutableMemory, normal use", [](etest::IActions &a) {
        // MOV EAX, 42 ; b8 2a 0 0 0
        // RET         ; c3
        auto exec_memory = os::ExecutableMemory::allocate_containing({{0xb8, 0x2a, 0, 0, 0, 0xc3}});
        a.require(exec_memory.has_value());
#ifdef AMD64
        using Fn = std::int32_t (*)();
        auto get_42 = reinterpret_cast<Fn>(exec_memory->ptr());
        a.expect_eq(get_42(), 42);
#endif
    });

    s.add_test("ExecutableMemory, empty data", [](etest::IActions &a) {
        a.expect_eq(os::ExecutableMemory::allocate_containing({}), std::nullopt); //
    });

    return s.run();
}
