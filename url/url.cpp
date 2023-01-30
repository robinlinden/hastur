// SPDX-FileCopyrightText: 2023 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "url/url.h"

#include "util/string.h"
#include "util/uuid.h"

#include <array>
#include <cstdint>
#include <string>
#include <variant>

namespace url {

// https://w3c.github.io/FileAPI/#unicodeBlobURL
std::string blob_url_create(Origin const &origin) {
    std::string result = "blob:";
    std::string serialized = "";

    // https://html.spec.whatwg.org/multipage/browsers.html#ascii-serialisation-of-an-origin
    if (origin.opaque) {
        serialized = "null";
    } else {
        serialized = origin.scheme + "://";

        switch (origin.host.type) {
            case HostType::DnsDomain:
            case HostType::Opaque:
            case HostType::Empty:
                serialized += std::get<std::string>(origin.host.data);
                break;
            case HostType::Ip4Addr:
                serialized += util::ipv4_serialize(std::get<std::uint32_t>(origin.host.data));
                break;
            case HostType::Ip6Addr:
                std::array<std::uint16_t, 8> v6 = std::get<std::array<std::uint16_t, 8>>(origin.host.data);
                serialized += "[" + util::ipv6_serialize(v6) + "]";
        }

        if (origin.port.has_value()) {
            serialized += ":" + std::to_string(origin.port.value());
        }
    }

    result += serialized;
    result += "/";
    result += util::new_uuid();

    return result;
}

} // namespace url
