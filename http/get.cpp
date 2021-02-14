#include "http/get.h"

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <sstream>
#include <string>
#include <string_view>
#include <utility>

using namespace std::string_view_literals;

namespace http {
namespace {

std::pair<std::string_view, std::string_view> split(std::string_view str, std::string_view sep) {
    if (auto it = str.find(sep); it != std::string::npos) {
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

    if (protocol == "http"sv) {
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

    if (protocol == "https"sv) {
        asio::io_service svc;
        asio::ssl::context ctx{asio::ssl::context::method::sslv23_client};
        asio::ssl::stream<asio::ip::tcp::socket> ssock(svc, ctx);

        asio::ip::tcp::resolver resolver{svc};
        auto endpoints = resolver.resolve(endpoint, "https"sv);

        ssock.lowest_layer().connect(*endpoints.begin());
        ssock.handshake(asio::ssl::stream_base::handshake_type::client);

        std::stringstream ss;
        ss << "GET / HTTP/1.1\r\n";
        ss << fmt::format("Host: {}\r\n", endpoint);
        ss << "Accept: text/html\r\n";
        ss << "Connection: close\r\n\r\n";
        asio::error_code ec;
        asio::write(ssock, asio::buffer(ss.str()), ec);

        std::string data;
        while (true) {
            char buf[1024];
            std::size_t received = ssock.read_some(asio::buffer(buf), ec);
            if (ec) { break; }
            data.append(buf, buf + received);
        }

        auto [header, body] = split(data, "\r\n\r\n");
        return {std::string{header}, std::string{body}};
    }

    spdlog::error("http::get: Unhandled protocol '{}'", protocol);
    return {};
}

} // namespace http
