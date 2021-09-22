// SPDX-FileCopyrightText: 2021 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef URI_URI_H_
#define URI_URI_H_

#include <optional>
#include <string>

namespace uri {

struct Authority {
    std::string user;
    std::string passwd;
    std::string host;
    std::string port;

    bool operator==(Authority const &) const = default;
};

struct Uri {
    static std::optional<Uri> parse(std::string uri);

    std::string uri;
    std::string scheme;
    Authority authority;
    std::string path;
    std::string query;
    std::string fragment;

    bool operator==(Uri const &) const = default;
};

} // namespace uri

#endif
