// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021-2022 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/response.h"

#include "util/string.h"

#include <algorithm>
#include <sstream>
#include <utility>

namespace protocol {

void Headers::add(std::pair<std::string_view, std::string_view> nv) {
    headers_.emplace(nv);
}

std::optional<std::string_view> Headers::get(std::string_view name) const {
    auto it = headers_.find(name);
    if (it != cend(headers_)) {
        return it->second;
    }
    return std::nullopt;
}
std::string Headers::to_string() const {
    std::stringstream ss{};
    for (auto const &[name, value] : headers_) {
        ss << name << ": " << value << "\n";
    }
    return std::move(ss).str();
}

std::size_t Headers::size() const {
    return headers_.size();
}

bool Headers::CaseInsensitiveLess::operator()(std::string_view s1, std::string_view s2) const {
    return std::ranges::lexicographical_compare(
            s1, s2, [](char c1, char c2) { return util::lowercased(c1) < util::lowercased(c2); });
}

} // namespace protocol
