#include <asio.hpp>

#include <iostream>

int main(int argc, char **argv) {
    asio::ip::tcp::iostream stream("www.example.com", "http");
    stream << "GET / HTTP/1.1\r\n";
    stream << "Host: www.example.com\r\n";
    stream << "Accept: text/html\r\n";
    stream << "Connection: close\r\n\r\n";
    stream.flush();
    std::cout << stream.rdbuf();
}
