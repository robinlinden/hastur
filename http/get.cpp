#include "http/get.h"

#include <asio.hpp>
#include <fmt/format.h>

#include <sstream>
#include <string>

namespace http {

Response get(std::string_view endpoint) {
    asio::ip::tcp::iostream stream(endpoint, "http");
    stream << "GET / HTTP/1.1\r\n";
    stream << fmt::format("Host: {}\r\n", endpoint);
    stream << "Accept: text/html\r\n";
    stream << "Connection: close\r\n\r\n";
    stream.flush();

    std::stringstream ss;
    ss << stream.rdbuf();
    std::string data{ss.str()};

    auto it{data.find("\r\n\r\n")};
    return {{begin(data), begin(data) + it}, {begin(data) + it, end(data)}};
}

} // namespace http
