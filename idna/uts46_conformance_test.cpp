// SPDX-FileCopyrightText: 2024-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "idna/uts46.h"

#include "etest/etest2.h"
#include "util/string.h"

#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>

int main(int argc, char **argv) {
    if (argc != 2) {
        char const *program_name = argv[0] != nullptr ? argv[0] : "<bin>";
        std::cerr << "Usage: " << program_name << " <path/to/IdnaTestV2.txt>\n";

        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Failed to open file: " << argv[1] << '\n';
        return 1;
    }

    etest::Suite s;

    // For details about the file format, see the comment at the top of IdnaTestV2.txt.
    for (std::string line; std::getline(file, line);) {
        if (line.empty() || line.starts_with('#')) {
            continue;
        }

        auto cols = util::split(line, ";");
        if (cols.size() != 7) {
            std::cerr << "Invalid line: " << line << '\n';
            return 1;
        }

        auto src = std::string{util::trim(cols[0])};
        auto to_unicode_res = std::string{util::trim(cols[1])};
        if (to_unicode_res.empty()) {
            to_unicode_res = src;
        } else if (to_unicode_res == "\"\"") {
            to_unicode_res = "";
        }

        // TODO(robinlinden): Handle these properly instead of skipping them.
        if (src.contains("\\u") || to_unicode_res.contains("\\u") || src == "ꖨ.16.3툒۳") {
            continue;
        }

        auto to_unicode_status = util::trim(cols[2]);
        // TODO(robinlinden): Handle these properly instead of skipping them.
        if (to_unicode_status == "[B5]" || to_unicode_status == "[V6]") {
            continue;
        }

        auto to_ascii_res = std::string{util::trim(cols[3])};
        if (to_ascii_res.empty()) {
            to_ascii_res = to_unicode_res;
        }

        s.add_test("to_unicode: " + src, [s = src, res = std::move(to_unicode_res)](etest::IActions &a) {
            auto processed = idna::Uts46::to_unicode(s);
            // TODO(robinlinden): Don't skip these.
            if (!processed) {
                return;
            }

            a.expect_eq(res,
                    processed,
                    std::format("'{}' !=\n'{}', actually:\n'{}'", s, res, processed.value_or("<invalid>")));
        });

        s.add_test("to_ascii: " + src, [s = src, res = std::move(to_ascii_res)](etest::IActions &a) {
            auto processed = idna::Uts46::to_ascii(s);
            // TODO(robinlinden): Don't skip these.
            if (!processed) {
                return;
            }

            a.expect_eq(res,
                    processed,
                    std::format("'{}' !=\n'{}', actually:\n'{}'", s, res, processed.value_or("<invalid>")));
        });
    }

    return s.run();
}
