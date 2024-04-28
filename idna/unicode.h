// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef IDNA_UNICODE_H_
#define IDNA_UNICODE_H_

#include <string>
#include <string_view>

namespace idna {

class Unicode {
public:
    // Normalizes the input into its canonical decomposition, NFD.
    static std::string decompose(std::string_view);
};

} // namespace idna

#endif
