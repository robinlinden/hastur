// SPDX-FileCopyrightText: 2023 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "net/ip.h"

#include <fmt/core.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <span>

namespace net {

// https://url.spec.whatwg.org/#concept-ipv4-serializer
std::string ipv4_serialize(std::uint32_t addr) {
    std::string out = "";
    std::uint32_t n = addr;

    for (std::size_t i = 1; i < 5; i++) {
        out.insert(0, std::to_string(n % 256));

        if (i != 4) {
            out.insert(0, ".");
        }

        n >>= 8;
    }

    return out;
}

// https://url.spec.whatwg.org/#concept-ipv6-serializer
std::string ipv6_serialize(std::span<std::uint16_t, 8> addr) {
    std::string out = "";

    std::size_t compress = 0;

    std::size_t longest_run = 1;
    std::size_t run = 1;

    // Set compress to the index of the longest run of 0 pieces
    for (std::size_t i = 1; i < 8; i++) {
        if (addr[i - 1] == 0 && addr[i] == 0) {
            run++;

            if (run > longest_run) {
                longest_run = run;
                compress = i - (run - 1);
            }
        } else {
            run = 1;
        }
    }

    bool ignore0 = false;

    for (std::size_t i = 0; i < 8; i++) {
        if (ignore0 && addr[i] == 0) {
            continue;
        } else if (ignore0) {
            ignore0 = false;
        }

        if (longest_run > 1 && compress == i) {
            if (i == 0) {
                out += "::";
            } else {
                out += ":";
            }

            ignore0 = true;

            continue;
        }

        out += fmt::format("{:x}", addr[i]);

        if (i != 7) {
            out += ":";
        }
    }

    return out;
}

} // namespace net
