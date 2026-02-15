// SPDX-FileCopyrightText: 2023-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef URL_RTTI_HACK_H_
#define URL_RTTI_HACK_H_

#include <unicode/bytestream.h>

#include <string>

// icu needs to be compiled w/ rtti, and that means that any templates of theirs
// that we instantiate also require rtti, so we instantiate them here to try to
// shield the rest of the codebase from that.
//
// portability-template-virtual-member-function: While this is gross, we test
// this with all compilers we support, and the plan is for this to go away
// together with icu soon.
extern template class icu::StringByteSink<std::string>; // NOLINT(portability-template-virtual-member-function)

#endif
