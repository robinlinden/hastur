#include <asio.hpp>
#include <pugixml.hpp>

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>

using namespace std::string_literals;

std::string drop_http_headers(std::string html) {
    const auto delim = "\r\n\r\n"s;
    auto it = html.find(delim);
    html.erase(0, it + delim.size());
    return html;
}

std::string drop_head(std::string html) {
    const auto tag_start = "<head>"s;
    const auto tag_end = "</head>"s;
    auto head = html.find(tag_start);
    html.erase(head, html.find(tag_end) - head + tag_end.size());
    return html;
}

std::string drop_doctype(std::string html) {
    html.erase(0, "<!doctype html>"s.size());
    return html;
}

int main(int argc, char **argv) {
    asio::ip::tcp::iostream stream("www.example.com", "http");
    stream << "GET / HTTP/1.1\r\n";
    stream << "Host: www.example.com\r\n";
    stream << "Accept: text/html\r\n";
    stream << "Connection: close\r\n\r\n";
    stream.flush();

    std::stringstream ss;
    ss << stream.rdbuf();
    auto buffer = ss.str();

    buffer = drop_http_headers(buffer);
    buffer = drop_head(buffer);
    buffer = drop_doctype(buffer);

    pugi::xml_document doc;
    if (auto res = doc.load_string(buffer.c_str()); !res) {
        std::cerr << res.offset << ": " << res.description() << '\n';
        std::cerr << buffer.c_str() + res.offset;
        return 1;
    }

    doc.print(std::cout);
}
