// SPDX-FileCopyrightText: 2021 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "uri/uri.h"

#include <regex>
#include <utility>

namespace uri {
namespace {

// https://en.wikipedia.org/wiki/URI_normalization#Normalization_process
void normalize(Uri &uri) {
    // In presence of an authority component, an empty path component should be
    // normalized to a path component of "/".
    if (!uri.authority.empty() && uri.path.empty()) {
        uri.path = "/";
    }
}

} // namespace

std::optional<Uri> Uri::parse(std::string uristr) {
    std::smatch match;

    // Regex taken from RFC 3986.
    std::regex uri_regex("^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?");
    if (!std::regex_search(uristr, match, uri_regex)) {
        return std::nullopt;
    }

    Authority authority{};

    std::string hostport = match.str(4);
    if (auto userinfo_end = match.str(4).find_first_of('@'); userinfo_end != std::string::npos) {
        // Userinfo present.
        std::string userinfo(match.str(4).substr(0, userinfo_end));
        hostport = match.str(4).substr(userinfo_end + 1, match.str(4).size() - userinfo_end);

        if (auto user_end = userinfo.find_first_of(':'); user_end != std::string::npos) {
            // Password present.
            authority.user = userinfo.substr(0, user_end);
            authority.passwd = userinfo.substr(user_end + 1, userinfo.size() - user_end);
        } else {
            // Password not present.
            authority.user = userinfo;
        }
    }

    if (auto host_end = hostport.find_first_of(':'); host_end != std::string::npos) {
        // Port present.
        authority.host = hostport.substr(0, host_end);
        authority.port = hostport.substr(host_end + 1, hostport.size() - host_end);
    } else {
        // Port not present.
        authority.host = hostport;
    }

    auto uri = Uri{
            .scheme{match.str(2)},
            .authority{std::move(authority)},
            .path{match.str(5)},
            .query{match.str(7)},
            .fragment{match.str(9)},
    };
    uri.uri = std::move(uristr);

    normalize(uri);

    return uri;
}

} // namespace uri
