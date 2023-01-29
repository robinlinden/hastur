// SPDX-FileCopyrightText: 2023 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef URL_URL_H_
#define URL_URL_H_

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>

namespace url {

enum class HostType { DnsDomain, Ip4Addr, Ip6Addr, Opaque, Empty };

struct Host {
    HostType type;

    std::variant<std::string, std::uint32_t, std::array<std::uint16_t, 8>> data;
};

struct Origin {
    std::string scheme;
    Host host;
    std::optional<std::uint16_t> port;
    std::optional<std::string> domain;
    // Need this placeholder until I figure out what "opaqueness" means for an origin in this context
    bool opaque;
};

/**
 * Generates a new Blob URL for the given origin
 */
std::string blob_url_create(Origin const &origin);

} // namespace url

#endif
