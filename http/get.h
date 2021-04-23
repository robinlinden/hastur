#ifndef HTTP_GET_H_
#define HTTP_GET_H_

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

Response get(std::string_view endpoint);

} // namespace http

#endif
