// SPDX-FileCopyrightText: 2021 Mikael Larsson <c.mikael.larsson@gmail.com>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "protocol/http.h"

#include "etest/etest.h"

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

    std::string read_all() { return read_data; }

    std::size_t read_until(std::string &data, std::string_view d) {
        delimiter = d;
        data = read_data;
        return read_until_pos;
    }

    std::string host{};
    std::string service{};
    std::string write_data{};
    std::string read_data{};
    std::string delimiter{};
    std::size_t read_until_pos{};
    bool connect_result{true};
};

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

        auto response = protocol::Http::get(socket, uri::Uri::parse("http://example.com").value());

        require(response.headers.size() == 13);
        expect_eq(socket.host, "example.com");
        expect_eq(socket.service, "http");
        expect_eq(response.status_line.version, "HTTP/1.1");
        expect_eq(response.status_line.status_code, 200);
        expect_eq(response.status_line.reason, "OK");
        expect_eq(response.headers["Content-Encoding"], "gzip");
        expect_eq(response.headers["Accept-Ranges"], "bytes");
        expect_eq(response.headers["Age"], "367849");
        expect_eq(response.headers["Cache-Control"], "max-age=604800");
        expect_eq(response.headers["Content-Type"], "text/html; charset=UTF-8");
        expect_eq(response.headers["Date"], "Mon, 25 Oct 2021 19:48:04 GMT");
        expect_eq(response.headers["Etag"], "\"3147526947\"");
        expect_eq(response.headers["Expires"], "Mon, 01 Nov 2021 19:48:04 GMT");
        expect_eq(response.headers["Last-Modified"], "Thu, 17 Oct 2019 07:18:26 GMT");
        expect_eq(response.headers["Server"], "ECS (nyb/1D2A)");
        expect_eq(response.headers["Vary"], "Accept-Encoding");
        expect_eq(response.headers["X-Cache"], "HIT");
        expect_eq(response.headers["Content-Length"], "123");
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

        auto response = protocol::Http::get(socket, uri::Uri::parse("http://google.com").value());

        require(response.headers.size() == 7);
        expect_eq(socket.host, "google.com");
        expect_eq(socket.service, "http");
        expect_eq(response.status_line.version, "HTTP/1.1");
        expect_eq(response.status_line.status_code, 301);
        expect_eq(response.status_line.reason, "Moved Permanently");
    });

    // TODO(mkiael): Fix so that this test passes
    // etest::test("404 no headers no body", [] {
    //    auto response = protocol::parse("HTTP/1.1 404 Not Found\r\n\r\n"sv);
    //
    //    require(response.headers.size() == 0);
    //    expect_eq(response.status_line.version, "HTTP/1.1"sv);
    //    expect_eq(response.status_line.status_code, 404);
    //    expect_eq(response.status_line.reason, "Not Found");
    //});

    return etest::run_all_tests();
}