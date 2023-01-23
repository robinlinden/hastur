// SPDX-FileCopyrightText: 2021 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "uri/uri.h"

#include "net/ip.h"
#include "util/string.h"
#include "util/uuid.h"

#include <ctre.hpp>
#include <fmt/format.h>

#include <exception>
#include <optional>
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
        uri = Uri::parse(fmt::format("{}://{}{}", base.scheme, base.authority.host, uri.uri));
    } else if (uri.scheme.empty() && uri.authority.host.empty() && !uri.path.empty()) {
        // https://url.spec.whatwg.org/#path-relative-url-string
        uri = Uri::parse(fmt::format("{}/{}", base.uri, uri.uri));
    } else if (uri.scheme.empty() && !uri.authority.host.empty() && uri.uri.starts_with("//")) {
        // Scheme-relative.
        uri = Uri::parse(fmt::format("{}:{}", base.scheme, uri.uri));
    }
}

} // namespace

Uri Uri::parse(std::string uristr, std::optional<std::reference_wrapper<Uri const>> base_uri) {
    // Regex taken from RFC 3986.
    auto match = ctre::match<"^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?">(uristr);
    if (!match) {
        std::terminate();
    }

    Authority authority{};

    std::string hostport = match.get<4>().str();
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
            .scheme{match.get<2>().str()},
            .authority{std::move(authority)},
            .path{match.get<5>().str()},
            .query{match.get<7>().str()},
            .fragment{match.get<9>().str()},
    };
    uri.uri = std::move(uristr);

    normalize(uri);

    if (base_uri) {
        complete_from_base_if_needed(uri, base_uri->get());
    }

    return uri;
}

// https://w3c.github.io/FileAPI/#unicodeBlobURL
std::string blob_url_create(Origin *origin) {
    if (origin == NULL) {
        return "";
    }

    std::string result = "blob:";
    std::string serialized = "";

    // https://html.spec.whatwg.org/multipage/browsers.html#ascii-serialisation-of-an-origin
    if (origin->opaque) {
        serialized = "null";
    } else {
        serialized = origin->scheme + "://";

        switch (origin->host->type) {
            case DNS_DOMAIN:
            case OPAQUE:
            case EMPTY:
                serialized += origin->host->domain;
                break;
            case IP4_ADDR:
                serialized += net::ipv4_serialize(origin->host->ip4_addr);
                break;
            case IP6_ADDR:
                serialized += "[" + net::ipv6_serialize(origin->host->ip6_addr) + "]";
        }

        if (origin->port.has_value()) {
            serialized += ":" + std::to_string(origin->port.value());
        }
    }

    result += serialized;
    result += "/";
    result += util::new_uuid();

    return result;
}

bool URL::parse(std::optional<URL> base) {
    bool ret = parse_basic(base, std::nullopt, std::nullopt);

    if (ret) {
        return true;
    }

    return true;
}

ParserState URL::parse_basic(
        std::optional<URL> base, std::optional<URL> url, std::optional<ParserState> state_override) {
    if (base || url || state_override) {
        return FAILURE;
    }

    return FAILURE;
}

} // namespace uri
