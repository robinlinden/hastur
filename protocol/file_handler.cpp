// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/file_handler.h"

#include "protocol/response.h"

#include "uri/uri.h"

#include <tl/expected.hpp>

#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>
#include <system_error>
#include <utility>

namespace protocol {
namespace {

tl::expected<Response, Error> handle_folder_request(std::filesystem::path const &path) {
    Response response{};

    // TODO(robinlinden): Only show '..' if we can navigate up. Will be more
    // convenient to deal with once we've switched to the spec-compliant url
    // implementation.
    // Allow navigation up in the directory structure.
    response.body += "<a href=\"../\">../</a></br>\n";

    auto body = std::back_inserter(response.body);

    for (auto const &entry : std::filesystem::directory_iterator(path)) {
        auto path_str = entry.path().string();
        auto filename = entry.path().filename().string();

        if (entry.is_directory()) {
            std::format_to(body, "<a href=\"{}/\">{}/</a></br>\n", path_str, filename);
            continue;
        }

        std::format_to(body, "<a href=\"{}\">{}</a></br>\n", path_str, filename);
    }

    return response;
}

} // namespace

tl::expected<Response, Error> FileHandler::handle(uri::Uri const &uri) {
    auto path = std::filesystem::path(uri.path);
    std::error_code ec;
    auto type = status(path, ec).type();
    if (ec && ec != std::errc::no_such_file_or_directory) {
        return tl::unexpected{protocol::Error{ErrorCode::InvalidResponse}};
    }

    if (type == std::filesystem::file_type::not_found) {
        return tl::unexpected{protocol::Error{ErrorCode::Unresolved}};
    }

    if (type == std::filesystem::file_type::directory) {
        return handle_folder_request(path);
    }

    if (type != std::filesystem::file_type::regular) {
        return tl::unexpected{protocol::Error{ErrorCode::InvalidResponse}};
    }

    auto file = std::ifstream(path, std::ios::in | std::ios::binary);
    auto size = file_size(path);
    auto content = std::string(size, '\0');
    file.read(content.data(), size);
    return Response{{}, {}, std::move(content)};
}

} // namespace protocol
