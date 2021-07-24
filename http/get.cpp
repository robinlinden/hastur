#include "http/get.h"

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <fmt/format.h>

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

Response get(uri::Uri const &uri) {
    if (uri.scheme == "http"sv) {
        asio::ip::tcp::iostream stream(uri.authority.host, "http"sv);
        stream << fmt::format("GET {} HTTP/1.1\r\n", uri.path);
        stream << fmt::format("Host: {}\r\n", uri.authority.host);
        stream << "Accept: text/html\r\n";
        stream << "Connection: close\r\n\r\n";
        stream.flush();

        std::stringstream ss;
        ss << stream.rdbuf();
        std::string data{ss.str()};

        auto [header, body] = split(data, "\r\n\r\n");
        return {Error::Ok, std::string{header}, std::string{body}};
    }

    if (uri.scheme == "https"sv) {
        asio::io_service svc;
        asio::ssl::context ctx{asio::ssl::context::method::sslv23_client};
        asio::ssl::stream<asio::ip::tcp::socket> ssock(svc, ctx);
        asio::error_code ec;

        asio::ip::tcp::resolver resolver{svc};
        auto endpoints = resolver.resolve(uri.authority.host, "https"sv, ec);
        if (ec) {
            return {Error::Unresolved};
        }

        ssock.lowest_layer().connect(*endpoints.begin());
        ssock.handshake(asio::ssl::stream_base::handshake_type::client);

        std::stringstream ss;
        ss << fmt::format("GET {} HTTP/1.1\r\n", uri.path);
        ss << fmt::format("Host: {}\r\n", uri.authority.host);
        ss << "Accept: text/html\r\n";
        ss << "Connection: close\r\n\r\n";
        asio::write(ssock, asio::buffer(ss.str()), ec);

        std::string data;
        while (true) {
            char buf[1024];
            std::size_t received = ssock.read_some(asio::buffer(buf), ec);
            if (ec) { break; }
            data.append(buf, buf + received);
        }

        auto [header, body] = split(data, "\r\n\r\n");
        return {Error::Ok, std::string{header}, std::string{body}};
    }

    return {Error::Unhandled};
}

} // namespace http
