// SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef IDNA_UTS46_H_
#define IDNA_UTS46_H_

#include <optional>
#include <string>
#include <string_view>

namespace idna {

class Uts46 {
public:
    static std::optional<std::string> map(std::string_view);
};

} // namespace idna

#endif
