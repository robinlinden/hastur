// SPDX-FileCopyrightText: 2021 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef PROTOCOL_GET_H_
#define PROTOCOL_GET_H_

#include "protocol/http.h"
#include "uri/uri.h"

namespace protocol {

Response get(uri::Uri const &uri);

} // namespace protocol

#endif
