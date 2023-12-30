// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "url/rtti_hack.h" // IWYU pragma: keep

#include <unicode/bytestream.h>

#include <string>

template class icu::StringByteSink<std::string>;
