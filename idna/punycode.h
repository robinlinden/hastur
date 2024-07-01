// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef IDNA_PUNYCODE_H_
#define IDNA_PUNYCODE_H_

#include "unicode/util.h"

#include <algorithm>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace idna {

// https://datatracker.ietf.org/doc/html/rfc3492
class Punycode {
public:
    // https://datatracker.ietf.org/doc/html/rfc3492#section-6.2
    static constexpr std::optional<std::string> to_utf8(std::string_view encoded) {
        int n = kInitialN;
        int i = 0;
        int bias = kInitialBias;
        std::u32string output{};

        if (auto last_delimiter = encoded.find_last_of(kDelimiter); last_delimiter != std::string_view::npos) {
            // No need to turn these into code points, since they are all ASCII.
            for (auto cp : encoded.substr(0, last_delimiter)) {
                output.push_back(cp);
            }

            encoded.remove_prefix(last_delimiter + 1);

            if (std::ranges::any_of(output, std::not_fn(&is_basic_code_point))) {
                return std::nullopt;
            }
        }

        // clang-tidy says this is pointer-ish, but msvc disagrees.
        // NOLINTNEXTLINE(readability-qualified-auto)
        auto input = encoded.begin();
        while (input != encoded.end()) {
            int oldi = i;
            int w = 1;
            for (int k = kBase; true; k += kBase) {
                if (input == encoded.end()) {
                    return std::nullopt;
                }

                auto digit = digit_value(*input++).value_or(-1);
                if (digit == -1) {
                    return std::nullopt;
                }

                // TODO(robinlinden): Fail on overflow.
                i += digit * w;
                int t = [&] {
                    if (k <= bias) {
                        return kTMin;
                    }

                    if (k >= bias + kTMax) {
                        return kTMax;
                    }

                    return k - bias;
                }();

                if (digit < t) {
                    break;
                }

                // TODO(robinlinden): Fail on overflow.
                w *= kBase - t;
            }

            bias = adapt(i - oldi, static_cast<int>(output.size()) + 1, oldi == 0);
            n += i / (static_cast<int>(output.size()) + 1);
            i %= output.size() + 1;
            output.insert(output.begin() + i, n);
            i += 1;
        }

        return unicode_to_utf8(output);
    }

private:
    // Parameter values for Punycode
    // https://datatracker.ietf.org/doc/html/rfc3492#section-5
    static constexpr int kBase = 36;
    static constexpr int kTMin = 1;
    static constexpr int kTMax = 26;
    static constexpr int kSkew = 38;
    static constexpr int kDamp = 700;
    static constexpr int kInitialBias = 72;
    static constexpr int kInitialN = 128;

    static constexpr bool is_basic_code_point(char32_t cp) { return cp < 0x80; }

    static constexpr char32_t kDelimiter = '-';

    static constexpr std::optional<int> digit_value(char32_t cp) {
        if (cp >= 'A' && cp <= 'Z') {
            return cp - 'A';
        }

        if (cp >= 'a' && cp <= 'z') {
            return cp - 'a';
        }

        if (cp >= '0' && cp <= '9') {
            return cp - '0' + 26;
        }

        return std::nullopt;
    }

    // https://datatracker.ietf.org/doc/html/rfc3492#section-6.1
    static constexpr int adapt(int delta, int numpoints, bool firsttime) {
        delta = firsttime ? delta / kDamp : delta / 2;
        delta += delta / numpoints;

        int k = 0;
        while (delta > ((kBase - kTMin) * kTMax) / 2) {
            delta /= kBase - kTMin;
            k += kBase;
        }

        return k + ((kBase - kTMin + 1) * delta) / (delta + kSkew);
    }

    static constexpr std::string unicode_to_utf8(std::u32string const &code_points) {
        std::string result{};
        for (auto const code_point : code_points) {
            result += unicode::to_utf8(code_point);
        }

        return result;
    }
};

} // namespace idna

#endif
