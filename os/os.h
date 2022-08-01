// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef OS_OS_H_
#define OS_OS_H_

#include <string>
#include <vector>

namespace os {

std::vector<std::string> font_paths();
unsigned active_window_scale_factor();

} // namespace os

#endif
