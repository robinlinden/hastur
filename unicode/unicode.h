// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef UNICODE_UNICODE_H_
#define UNICODE_UNICODE_H_

#include <string>
#include <string_view>

namespace unicode {

class Unicode {
public:
    // Normalizes the input into its canonical decomposition, NFD.
    static std::string decompose(std::string_view);
};

} // namespace unicode

#endif
