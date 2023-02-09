// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef URL_RTTI_HACK_H_
#define URL_RTTI_HACK_H_

#include <unicode/bytestream.h>

#include <string>

// icu needs to be compiled w/ rtti, and that means that any templates of theirs
// that we instantiate also require rtti, so we instantiate them here to try to
// shield the rest of the codebase from that.
extern template class icu::StringByteSink<std::string>;

#endif
