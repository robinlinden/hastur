#ifndef HTTP_GET_H_
#define HTTP_GET_H_

#include "uri/uri.h"

#include <string>
#include <string_view>

namespace http {

enum class Error {
    Ok,
    Unresolved,
    Unhandled,
};

struct Response {
    Error err;
    std::string header;
    std::string body;
};

Response get(uri::Uri const &uri);

} // namespace http

#endif
