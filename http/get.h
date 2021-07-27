#ifndef HTTP_GET_H_
#define HTTP_GET_H_

#include "uri/uri.h"

#include <map>
#include <string>
#include <string_view>

namespace http {

enum class Error {
    Ok,
    Unresolved,
    Unhandled,
    InvalidResponse,
};

struct StatusLine {
    std::string version;
    int status_code;
    std::string reason;
};

struct Response {
    Error err;
    StatusLine status_line;
    std::map<std::string, std::string> headers;
    std::string body;
};

Response get(uri::Uri const &uri);

std::string to_string(std::map<std::string, std::string> const &headers);

} // namespace http

#endif
