// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace archive {

// https://giflib.sourceforge.net/gifstandard/LZW-and-GIF-explained.html
struct Lzw {
    static constexpr std::optional<std::string> decompress(std::span<unsigned char const> compressed) {
        // [1] Initialize string table;
        std::vector<std::string> dictionary{};
        dictionary.resize(256);
        for (std::size_t i = 0; i < dictionary.size(); ++i) {
            dictionary[i] = std::string{static_cast<char>(i)};
        }

        if (compressed.empty() || compressed.front() > dictionary.size()) {
            return std::nullopt;
        }

        // [2] get first code: <code>
        // [3] output the string for <code> to the charstream;
        // [4] <old> = <code>
        std::string last_output = dictionary[compressed.front()];
        std::string result{last_output};
        compressed = compressed.subspan(1);

        // [5] <code> <- next code in codestream;
        for (auto next_code : compressed) {
            std::string next_output;
            // [6] does <code> exist in the string table?
            if (next_code < dictionary.size()) {
                // yes: output the string for <code> to the charstream;
                // K <- first character of translation for <code>
                // add [...]K to the string table;
                next_output = dictionary[next_code];
            } else if (next_code == dictionary.size()) {
                // no: [...] <- translation for <old>
                // K <- first character of [...];
                // output [...]K to charstream and add it to string table;
                next_output = last_output + last_output[0];
            } else {
                return std::nullopt;
            }

            result += next_output;
            dictionary.emplace_back(last_output + next_output[0]);
            // <old> <- <code>
            last_output = std::move(next_output);
            // [7] go to [5];
        }

        return result;
    }
};

} // namespace archive
