// SPDX-FileCopyrightText: 2021-2022 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/file_handler.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

namespace protocol {

Response FileHandler::handle(uri::Uri const &uri) {
    auto path = std::filesystem::path(uri.path);
    if (!exists(path)) {
        return {Error::Unresolved};
    }

    if (!is_regular_file(path)) {
        return {Error::InvalidResponse};
    }

    auto file = std::ifstream(path, std::ios::in | std::ios::binary);
    auto size = file_size(path);
    auto content = std::string(size, '\0');
    file.read(content.data(), size);
    return {Error::Ok, {}, {}, std::move(content)};
}

} // namespace protocol
