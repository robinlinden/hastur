#ifndef UTIL_URI_H_
#define UTIL_URI_H_

#include <optional>
#include <string>

namespace util {

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

} //namespace util

#endif
