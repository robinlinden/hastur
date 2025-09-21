// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2023 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef URL_IP_SERIALIZATION_H_
#define URL_IP_SERIALIZATION_H_

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <ios>
#include <span>
#include <sstream>
#include <string>
#include <utility>

namespace url {

// https://url.spec.whatwg.org/#concept-ipv4-serializer
constexpr std::string ipv4_serialize(std::uint32_t addr) {
    std::string out;
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
inline std::string ipv6_serialize(std::span<std::uint16_t const, 8> addr) {
    std::stringstream out;

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
        }

        if (ignore0) {
            ignore0 = false;
        }

        if (longest_run > 1 && compress == i) {
            if (i == 0) {
                out << "::";
            } else {
                out << ":";
            }

            ignore0 = true;

            continue;
        }

        out << std::hex << addr[i];

        if (i != 7) {
            out << ":";
        }
    }

    return std::move(out).str();
}

} // namespace url

#endif // URL_IP_SERIALIZATION_H_
