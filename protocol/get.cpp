// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/get.h"

#include "protocol/file_handler.h"
#include "protocol/http_handler.h"
#include "protocol/https_handler.h"

#include <string_view>

using namespace std::string_view_literals;

namespace protocol {

Response get(uri::Uri const &uri) {
    if (uri.scheme == "http"sv) {
        return HttpHandler{}.handle(uri);
    }

    if (uri.scheme == "https"sv) {
        return HttpsHandler{}.handle(uri);
    }

    if (uri.scheme == "file"sv) {
        return FileHandler{}.handle(uri);
    }

    return {Error::Unhandled};
}

} // namespace protocol
