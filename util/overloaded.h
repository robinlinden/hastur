// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef UTIL_OVERLOADED_H_
#define UTIL_OVERLOADED_H_

namespace util {

template<class... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};

// Not needed as of C++20, but Clang won't work without it.
template<class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

} // namespace util

#endif
