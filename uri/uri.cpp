// SPDX-FileCopyrightText: 2021 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2022-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "uri/uri.h"

#include "util/string.h"

#include <fmt/core.h>

#include <cassert>
#include <functional>
#include <optional>
#include <regex>
#include <string>
#include <utility>

namespace uri {
namespace {

// https://en.wikipedia.org/wiki/URI_normalization#Normalization_process
void normalize(Uri &uri) {
    // The scheme and host components of the URI are case-insensitive and
    // therefore should be normalized to lowercase.
    uri.scheme = util::lowercased(std::move(uri.scheme));
    uri.authority.host = util::lowercased(std::move(uri.authority.host));

    // In presence of an authority component, an empty path component should be
    // normalized to a path component of "/".
    if (!uri.authority.empty() && uri.path.empty()) {
        uri.path = "/";
    }
}

void complete_from_base_if_needed(Uri &uri, Uri const &base) {
    if (uri.scheme.empty() && uri.authority.host.empty() && uri.path.starts_with('/')) {
        // Origin-relative.
        auto new_uri = Uri::parse(fmt::format("{}://{}{}", base.scheme, base.authority.host, uri.uri));
        assert(new_uri.has_value());
        uri = *std::move(new_uri);
    } else if (uri.scheme.empty() && uri.authority.host.empty() && !uri.path.empty()) {
        // https://url.spec.whatwg.org/#path-relative-url-string
        if (base.path == "/") {
            auto new_uri = Uri::parse(fmt::format("{}/{}", base.uri, uri.uri));
            assert(new_uri);
            uri = *std::move(new_uri);
        } else {
            auto end_of_last_path_segment = base.uri.find_last_of('/');
            auto new_uri = Uri::parse(fmt::format("{}/{}", base.uri.substr(0, end_of_last_path_segment), uri.uri));
            assert(new_uri);
            uri = *std::move(new_uri);
        }
    } else if (uri.scheme.empty() && !uri.authority.host.empty() && uri.uri.starts_with("//")) {
        // Scheme-relative.
        auto new_uri = Uri::parse(fmt::format("{}:{}", base.scheme, uri.uri));
        assert(new_uri);
        uri = *std::move(new_uri);
    }
}

} // namespace

std::optional<Uri> Uri::parse(std::string uristr, std::optional<std::reference_wrapper<Uri const>> base_uri) {
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=86164
    // Fuzz-testing w/ libstdc++13 still breaks the stack if 2048 characters are allowed.
    if (uristr.size() > 1024) {
        return std::nullopt;
    }

    // Regex taken from RFC 3986.
    std::smatch match;
    std::regex const uri_regex{"^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?"};
    if (!std::regex_search(uristr, match, uri_regex, std::regex_constants::match_not_null)) {
        return std::nullopt;
    }

    Authority authority{};

    std::string hostport = match.str(4);
    if (auto userinfo_end = hostport.find_first_of('@'); userinfo_end != std::string::npos) {
        // Userinfo present.
        std::string userinfo = hostport.substr(0, userinfo_end);
        hostport = hostport.substr(userinfo_end + 1);

        if (auto user_end = userinfo.find_first_of(':'); user_end != std::string::npos) {
            // Password present.
            authority.user = userinfo.substr(0, user_end);
            authority.passwd = userinfo.substr(user_end + 1);
        } else {
            // Password not present.
            authority.user = std::move(userinfo);
        }
    }

    if (auto host_end = hostport.find_first_of(':'); host_end != std::string::npos) {
        // Port present.
        authority.host = hostport.substr(0, host_end);
        authority.port = hostport.substr(host_end + 1);
    } else {
        // Port not present.
        authority.host = std::move(hostport);
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

    if (base_uri) {
        complete_from_base_if_needed(uri, base_uri->get());
    }

    return uri;
}

} // namespace uri
