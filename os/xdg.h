// SPDX-FileCopyrightText: 2021-2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef OS_XDG_H_
#define OS_XDG_H_

#include <string>
#include <vector>

// TODO(robinlinden): We should probably create a more fully-featured top-level xdg library.
namespace os {
std::vector<std::string> font_paths();
} // namespace os

#endif
