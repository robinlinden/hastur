#include "util/uri.h"

#include <regex>
#include <utility>

namespace util {

std::optional<Uri> Uri::parse(std::string uristr){
    std::smatch match;

    // Regex taken from RFC 3986.
    std::regex uri_regex("^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?");
    if (!std::regex_search(uristr, match, uri_regex)) {
        return std::nullopt;
    }

    Authority authority{};

    std::string hostport = match.str(4);
    size_t userinfo_end = match.str(4).find_first_of("@");
    if (userinfo_end != std::string::npos) {
        // Userinfo present.
        std::string userinfo(match.str(4).substr(0, userinfo_end));
        hostport = match.str(4).substr(userinfo_end + 1, match.str(4).size() - userinfo_end);

        size_t user_end = userinfo.find_first_of(":");
        if (user_end != std::string::npos) {
            // Password present.
            authority.user = userinfo.substr(0, user_end);
            authority.passwd = userinfo.substr(user_end + 1, userinfo.size() - user_end);
        } else {
            // Password not present.
            authority.user = userinfo;
        }
    }

    size_t host_end = hostport.find_first_of(":");
    if (host_end != std::string::npos) {
        // Port present.
        authority.host = hostport.substr(0, host_end);
        authority.port = hostport.substr(host_end + 1, hostport.size() - host_end);
    } else {
        // Port not present.
        authority.host = hostport;
    }

    return Uri {
        .uri{std::move(uristr)},
        .scheme{match.str(2)},
        .authority{std::move(authority)},
        .path{match.str(5)},
        .query{match.str(7)},
        .fragment{match.str(9)},
    };
}

} // namespace util
