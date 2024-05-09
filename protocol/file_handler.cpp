// SPDX-FileCopyrightText: 2021-2024 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/file_handler.h"

#include "protocol/response.h"

#include "uri/uri.h"

#include <tl/expected.hpp>

#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <utility>

namespace protocol {

tl::expected<Response, Error> FileHandler::handle(uri::Uri const &uri) {
    auto path = std::filesystem::path(uri.path);
    if (!exists(path)) {
        return tl::unexpected{protocol::Error{ErrorCode::Unresolved}};
    }

    if (!is_regular_file(path)) {
        return tl::unexpected{protocol::Error{ErrorCode::InvalidResponse}};
    }

    auto file = std::ifstream(path, std::ios::in | std::ios::binary);
    auto size = file_size(path);
    auto content = std::string(size, '\0');
    file.read(content.data(), size);
    return Response{{}, {}, std::move(content)};
}

} // namespace protocol
