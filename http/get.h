#ifndef HTTP_GET_H_
#define HTTP_GET_H_

#include <string>
#include <string_view>

namespace http {

struct Response {
    std::string header;
    std::string body;
};

Response get(std::string_view endpoint);

} // namespace http

#endif
