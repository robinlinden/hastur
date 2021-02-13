#include "http/get.h"

#include <asio.hpp>
#include <fmt/format.h>

#include <sstream>
#include <string>
#include <string_view>
#include <utility>

using namespace std::string_view_literals;

namespace http {
namespace {

std::pair<std::string_view, std::string_view> split(std::string_view str, std::string_view sep) {
    if (auto it = str.find(sep); it != -1) {
        return {str.substr(0, it), str.substr(it + sep.size(), str.size() - it - sep.size())};
    }
    return {str, ""sv};
}

} // namespace

Response get(std::string_view url) {
    auto [protocol, endpoint] = split(url, "://"sv);
    if (endpoint.empty()) {
        // No protocol included, so let's assume http for now.
        endpoint = protocol;
        protocol = "http"sv;
    }

    asio::ip::tcp::iostream stream(endpoint, protocol);
    stream << "GET / HTTP/1.1\r\n";
    stream << fmt::format("Host: {}\r\n", endpoint);
    stream << "Accept: text/html\r\n";
    stream << "Connection: close\r\n\r\n";
    stream.flush();

    std::stringstream ss;
    ss << stream.rdbuf();
    std::string data{ss.str()};

    auto [header, body] = split(data, "\r\n\r\n");
    return {std::string{header}, std::string{body}};
}

} // namespace http
