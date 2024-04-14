// SPDX-FileCopyrightText: 2021-2022 Mikael Larsson <c.mikael.larsson@gmail.com>
// SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/http.h"

#include "etest/etest.h"
#include "protocol/response.h"
#include "uri/uri.h"

#include <cassert>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

using namespace std::string_view_literals;

using etest::expect_eq;
using etest::require;

namespace {

struct FakeSocket {
    bool connect(std::string_view h, std::string_view s) {
        host = h;
        service = s;
        return connect_result;
    }

    std::size_t write(std::string_view data) {
        write_data = data;
        return write_data.size();
    }

    std::string read_all() const { return read_data; }

    std::string read_until(std::string_view d) {
        delimiter = d;
        std::string result{};
        if (auto pos = read_data.find(d); pos != std::string::npos) {
            pos += d.size();
            result = read_data.substr(0, pos);
            read_data.erase(0, pos);
        }
        return result;
    }

    std::string read_bytes(std::size_t bytes) {
        std::string result = read_data.substr(0, bytes);
        read_data.erase(0, bytes);
        return result;
    }

    std::string host{};
    std::string service{};
    std::string write_data{};
    std::string read_data{};
    std::string delimiter{};
    bool connect_result{true};
};

uri::Uri create_uri(std::string url = "http://example.com") {
    auto parsed = uri::Uri::parse(std::move(url));
    assert(parsed.has_value());
    return std::move(parsed).value();
}

FakeSocket create_chunked_socket(std::string const &body) {
    FakeSocket socket;
    socket.read_data =
            "HTTP/1.1 200 OK\r\n"
            "Transfer-Encoding: chunked\r\n\r\n"
            + body;
    return socket;
}

} // namespace

int main() {
    etest::test("200 response", [] {
        FakeSocket socket;
        socket.read_data =
                "HTTP/1.1 200 OK\r\n"
                "Content-Encoding: gzip\r\n"
                "Accept-Ranges: bytes\r\n"
                "Age: 367849\r\n"
                "Cache-Control: max-age=604800\r\n"
                "Content-Type: text/html; charset=UTF-8\r\n"
                "Date: Mon, 25 Oct 2021 19:48:04 GMT\r\n"
                "Etag: \"3147526947\"\r\n"
                "Expires: Mon, 01 Nov 2021 19:48:04 GMT\r\n"
                "Last-Modified: Thu, 17 Oct 2019 07:18:26 GMT\r\n"
                "Server: ECS (nyb/1D2A)\r\n"
                "Vary: Accept-Encoding\r\n"
                "X-Cache: HIT\r\n"
                "Content-Length: 123\r\n"
                "\r\n"
                "<!doctype html>\n"
                "<html>\n"
                "<head>\n"
                "<title>Example Domain</title>\n"
                "</head>\n"
                "</html>\n";

        auto response = protocol::Http::get(socket, create_uri(), std::nullopt);

        require(response.headers.size() == 13);
        expect_eq(socket.host, "example.com");
        expect_eq(socket.service, "http");
        expect_eq(response.status_line.version, "HTTP/1.1");
        expect_eq(response.status_line.status_code, 200);
        expect_eq(response.status_line.reason, "OK");
        expect_eq(response.headers.get("Content-Encoding"sv).value(), "gzip");
        expect_eq(response.headers.get("Accept-Ranges"sv).value(), "bytes");
        expect_eq(response.headers.get("Age"sv).value(), "367849");
        expect_eq(response.headers.get("Cache-Control"sv).value(), "max-age=604800");
        expect_eq(response.headers.get("Content-Type"sv).value(), "text/html; charset=UTF-8");
        expect_eq(response.headers.get("Date"sv).value(), "Mon, 25 Oct 2021 19:48:04 GMT");
        expect_eq(response.headers.get("Etag"sv).value(), R"("3147526947")");
        expect_eq(response.headers.get("Expires"sv).value(), "Mon, 01 Nov 2021 19:48:04 GMT");
        expect_eq(response.headers.get("Last-Modified"sv).value(), "Thu, 17 Oct 2019 07:18:26 GMT");
        expect_eq(response.headers.get("Server"sv).value(), "ECS (nyb/1D2A)");
        expect_eq(response.headers.get("Vary"sv).value(), "Accept-Encoding");
        expect_eq(response.headers.get("X-Cache"sv).value(), "HIT");
        expect_eq(response.headers.get("Content-Length"sv).value(), "123");
        expect_eq(response.body,
                "<!doctype html>\n"
                "<html>\n"
                "<head>\n"
                "<title>Example Domain</title>\n"
                "</head>\n"
                "</html>\n");
    });

    etest::test("google 301", [] {
        FakeSocket socket;
        socket.read_data =
                "HTTP/1.1 301 Moved Permanently\r\n"
                "Location: http://www.google.com/\r\n"
                "Content-Type: text/html; charset=UTF-8\r\n"
                "Date: Sun, 26 Apr 2009 11:11:49 GMT\r\n"
                "Expires: Tue, 26 May 2009 11:11:49 GMT\r\n"
                "Cache-Control: public, max-age=2592000\r\n"
                "Server: gws\r\n"
                "Content-Length: 219\r\n"
                "\r\n"
                "<HTML><HEAD><meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\">\n"
                "<TITLE>301 Moved</TITLE></HEAD><BODY>\n"
                "<H1>301 Moved</H1>\n"
                "The document has moved\n"
                "<A HREF=\"http://www.google.com/\">here</A>.\r\n"
                "</BODY></HTML>\r\n";

        auto response = protocol::Http::get(socket, create_uri("http://google.com"), std::nullopt);

        require(response.headers.size() == 7);
        expect_eq(socket.host, "google.com");
        expect_eq(socket.service, "http");
        expect_eq(response.status_line.version, "HTTP/1.1");
        expect_eq(response.status_line.status_code, 301);
        expect_eq(response.status_line.reason, "Moved Permanently");
    });

    etest::test("transfer-encoding chunked, real body", [] {
        auto socket = create_chunked_socket(
                "7f\r\n"
                "<!DOCTYPE html>\r\n"
                "<html lang=en>\r\n"
                "<head>\r\n"
                "<meta charset='utf-8'>\r\n"
                "<title>Chunked transfer encoding test</title>\r\n"
                "</head>\r\n"
                "<body>\r\n"
                "27\r\n"
                "<h1>Chunked transfer encoding test</h1>\r\n"
                "31\r\n"
                "<h5>This is a chunked response after 100 ms.</h5>\r\n"
                "82\r\n"
                "<h5>This is a chunked response after 1 second. The server should not close the stream before all "
                "chunks are sent to a client.</h5>\r\n"
                "e\r\n"
                "</body></html>\r\n"
                "0\r\n"
                "\r\n");

        auto response = protocol::Http::get(socket, create_uri(), std::nullopt);

        expect_eq(response.body,
                "<!DOCTYPE html>\r\n"
                "<html lang=en>\r\n"
                "<head>\r\n"
                "<meta charset='utf-8'>\r\n"
                "<title>Chunked transfer encoding test</title>\r\n"
                "</head>\r\n"
                "<body><h1>Chunked transfer encoding test</h1><h5>This is a chunked response after 100 ms.</h5>"
                "<h5>This is a chunked response after 1 second. The server should not close the stream before all "
                "chunks are sent to a client.</h5></body></html>"sv);
    });

    etest::test("transfer-encoding chunked, space before size", [] {
        auto socket = create_chunked_socket(
                "  5\r\nhello\r\n"
                " 0\r\n\r\n");

        auto response = protocol::Http::get(socket, create_uri(), std::nullopt);

        expect_eq(response.body, "hello");
    });

    etest::test("transfer-encoding chunked, space after size", [] {
        auto socket = create_chunked_socket(
                "5  \r\nhello\r\n"
                "0  \r\n\r\n");

        auto response = protocol::Http::get(socket, create_uri(), std::nullopt);

        expect_eq(response.body, "hello");
    });

    etest::test("transfer-encoding chunked, invalid size", [] {
        auto socket = create_chunked_socket(
                "8684838388283847263674\r\nhello\r\n"
                "0\r\n\r\n");

        auto response = protocol::Http::get(socket, create_uri(), std::nullopt);

        expect_eq(response.err, protocol::Error::InvalidResponse);
    });

    etest::test("transfer-encoding chunked, no separator between chunk", [] {
        auto socket = create_chunked_socket(
                "5\r\nhello"
                "0\r\n\r\n");

        auto response = protocol::Http::get(socket, create_uri(), std::nullopt);

        expect_eq(response.err, protocol::Error::InvalidResponse);
    });

    etest::test("transfer-encoding chunked, chunk too short", [] {
        auto socket = create_chunked_socket(
                "6\r\nhello\r\n"
                "0\r\n\r\n");

        auto response = protocol::Http::get(socket, create_uri(), std::nullopt);

        expect_eq(response.err, protocol::Error::InvalidResponse);
    });

    etest::test("transfer-encoding chunked, chunk too long", [] {
        auto socket = create_chunked_socket(
                "3\r\nhello\r\n"
                "0\r\n\r\n");

        auto response = protocol::Http::get(socket, create_uri(), std::nullopt);

        expect_eq(response.err, protocol::Error::InvalidResponse);
    });

    etest::test("404 no headers no body", [] {
        FakeSocket socket;
        socket.read_data = "HTTP/1.1 404 Not Found\r\n\r\n";
        auto response = protocol::Http::get(socket, create_uri(), std::nullopt);

        require(response.headers.size() == 0);
        expect_eq(response.status_line.version, "HTTP/1.1"sv);
        expect_eq(response.status_line.status_code, 404);
        expect_eq(response.status_line.reason, "Not Found");
    });

    etest::test("connect failure", [] {
        FakeSocket socket{.connect_result = false};
        auto response = protocol::Http::get(socket, create_uri(), std::nullopt);
        expect_eq(response, protocol::Response{.err = protocol::Error::Unresolved});
    });

    etest::test("empty response", [] {
        FakeSocket socket{};
        auto response = protocol::Http::get(socket, create_uri(), std::nullopt);
        expect_eq(response, protocol::Response{.err = protocol::Error::InvalidResponse});
    });

    etest::test("empty status line", [] {
        FakeSocket socket{.read_data = "\r\n"};
        auto response = protocol::Http::get(socket, create_uri(), std::nullopt);
        expect_eq(response, protocol::Response{.err = protocol::Error::InvalidResponse});
    });

    etest::test("no headers", [] {
        FakeSocket socket{.read_data = "HTTP/1.1 200 OK\r\n \r\n\r\n"};
        auto response = protocol::Http::get(socket, create_uri(), std::nullopt);
        expect_eq(response,
                protocol::Response{.err = protocol::Error::InvalidResponse, .status_line{"HTTP/1.1", 200, "OK"}});
    });

    etest::test("mixed valid and invalid headers", [] {
        FakeSocket socket{.read_data = "HTTP/1.1 200 OK\r\none: 1\r\nBAD\r\ntwo:2 \r\n\r\n"};
        auto response = protocol::Http::get(socket, create_uri(), std::nullopt);
        expect_eq(response,
                protocol::Response{
                        .err = protocol::Error::Ok,
                        .status_line{"HTTP/1.1", 200, "OK"},
                        .headers{{"one", "1"}, {"two", "2"}},
                });
    });

    etest::test("query parameters are included", [] {
        FakeSocket socket{};
        auto uri = uri::Uri{
                .uri = {"http://example.com/hello?target=world"},
                .scheme = "http",
                .authority = {.host = "example.com"},
                .path = "/hello",
                .query = "target=world",
        };
        std::ignore = protocol::Http::get(socket, uri, std::nullopt);
        auto first_request_line = socket.write_data.substr(0, socket.write_data.find("\r\n"));
        expect_eq(first_request_line, "GET /hello?target=world HTTP/1.1");
    });

    return etest::run_all_tests();
}
