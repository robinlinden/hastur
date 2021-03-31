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
};

struct Uri {
    static std::optional<Uri> parse(std::string uri);

    std::string uri;
    std::string scheme;
    Authority authority;
    std::string path;
    std::string query;
    std::string fragment;
};

} //namespace util

#endif
