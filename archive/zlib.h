// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef ARCHIVE_ZLIB_H_
#define ARCHIVE_ZLIB_H_

#include <optional>
#include <string>
#include <string_view>

namespace archive {

std::optional<std::string> zlib_decode(std::string_view);

} // namespace archive

#endif
