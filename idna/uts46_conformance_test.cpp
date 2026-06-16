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

        auto src = util::trim(cols[0]);
        auto to_unicode_res = util::trim(cols[1]);
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

        s.add_test(std::string{src}, [s = std::string{src}, r = std::string{to_unicode_res}](etest::IActions &a) {
            auto processed = idna::Uts46::to_unicode(s);
            if (!processed) {
                return;
            }
            a.expect_eq(
                    r, processed, std::format("'{}' !=\n'{}', actually:\n'{}'", s, r, processed.value_or("<invalid>")));
        });
    }

    return s.run();
}
