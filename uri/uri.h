// SPDX-FileCopyrightText: 2021 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef URI_URI_H_
#define URI_URI_H_

#include "util/base_parser.h"

#include <functional>
#include <map>
#include <optional>
#include <span>
#include <string>

namespace uri {

struct Authority {
    std::string user;
    std::string passwd;
    std::string host;
    std::string port;

    [[nodiscard]] bool empty() const { return user.empty() && passwd.empty() && host.empty() && port.empty(); }

    [[nodiscard]] bool operator==(Authority const &) const = default;
};

struct Uri {
    static Uri parse(std::string uri, std::optional<std::reference_wrapper<Uri const>> base_uri = std::nullopt);

    std::string uri;
    std::string scheme;
    Authority authority;
    std::string path;
    std::string query;
    std::string fragment;

    [[nodiscard]] bool operator==(Uri const &) const = default;
};

enum HostType { DNS_DOMAIN, IP4_ADDR, IP6_ADDR, OPAQUE, EMPTY };

struct Host {
    HostType type;

    // union {
    std::string domain;
    std::uint32_t ip4_addr;
    std::uint16_t ip6_addr[8];
    //};
};

struct Origin {
    std::string scheme;
    Host *host;
    std::optional<uint16_t> port;
    std::optional<std::string> domain;
    // Need this placeholder until I figure out what "opaqueness" means for an origin in this context
    bool opaque;
};

enum ParserState {
    SCHEME_START,
    SCHEME,
    NO_SCHEME,
    SPECIAL_RELATIVE_OR_AUTHORITY,
    PATH_OR_AUTHORITY,
    RELATIVE,
    RELATIVE_SLASH,
    SPECIAL_AUTHORITY_SLASHES,
    SPECIAL_AUTHORITY_IGNORE_SLASHES,
    AUTHORITY,
    HOST,
    HOSTNAME,
    PORT,
    FILE,
    FILE_SLASH,
    FILE_HOST,
    PATH_START,
    PATH,
    OPAQUE_PATH,
    QUERY,
    FRAGMENT,
    FAILURE,
    TERMINATE
};

inline const std::map<std::string, std::string> special_schemes = {
        {"ftp", "21"}, {"file", ""}, {"http", "80"}, {"https", "443"}, {"ws", "80"}, {"wss", "443"}};

/**
 * Generates a new Blob URL for the given origin
 */
std::string blob_url_create(Origin *origin);

class URL final : util::BaseParser {
public:
    URL() : BaseParser{""} {}
    URL(std::string_view url, std::optional<URL> base = std::nullopt) : BaseParser{url} {
        this->input = std::string(url);

        parse(base);
    }

    std::string url_full;
    std::string scheme;
    std::string user;
    std::string passwd;
    std::string host;
    std::string port;
    std::string path;
    std::string query;
    std::string fragment;

private:
    bool parse(std::optional<URL> base);
    ParserState parse_basic(std::optional<URL> base, std::optional<URL> url, std::optional<ParserState> state_override);

    std::string input;
};

} // namespace uri

#endif
