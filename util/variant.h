// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef UTIL_VARIANT_H_
#define UTIL_VARIANT_H_

#include <cstddef>
#include <span>
#include <variant>

namespace util {

template<typename T, typename... Ts, typename... VariantTypesT, std::size_t SpanSizeT>
constexpr bool matches(std::span<std::variant<VariantTypesT...> const, SpanSizeT> variants) {
    if (variants.size() != sizeof...(Ts) + 1) {
        return false;
    }

    if constexpr (sizeof...(Ts) > 0) {
        return std::holds_alternative<T>(variants[0]) && matches<Ts...>(variants.subspan(1));
    } else {
        return std::holds_alternative<T>(variants[0]);
    }
}

} // namespace util

#endif
