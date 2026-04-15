// SPDX-FileCopyrightText: 2024-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef UNICODE_NORMALIZATION_H_
#define UNICODE_NORMALIZATION_H_

#include <string>
#include <string_view>

namespace unicode {

class Normalization {
public:
    // Normalizes the input into its canonical decomposition, NFD.
    static std::string nfd(std::string_view);
};

} // namespace unicode

#endif
