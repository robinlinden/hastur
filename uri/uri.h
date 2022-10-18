// SPDX-FileCopyrightText: 2021 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2022 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef URI_URI_H_
#define URI_URI_H_

#include <functional>
#include <optional>
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

} // namespace uri

#endif
