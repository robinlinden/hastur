// SPDX-FileCopyrightText: 2023 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "url/url.h"

#include "etest/etest2.h"
#include "json/json.h"

#include <format>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

int main(int argc, char **argv) {
    if (argc != 2) {
        auto exe = std::string_view{argv[0] != nullptr ? argv[0] : "<executable>"};
        std::cerr << "Usage: " << exe << " <path to urltestdata.json>\n";
        return 1;
    }

    std::ifstream test_file{argv[1], std::fstream::in | std::fstream::binary};
    if (!test_file) {
        std::cerr << "Failed to open test file '" << argv[1] << "'\n";
        return 1;
    }

    std::string urltestdata{std::istreambuf_iterator<char>(test_file), std::istreambuf_iterator<char>()};

    auto json = json::parse(urltestdata);
    if (!json) {
        std::cerr << "Error loading test file.\n";
        return 1;
    }

    auto const &arr = std::get<json::Array>(*json);

    etest::Suite s{};

    s.add_test("Web Platform Tests", [&](etest::IActions &a) {
        url::UrlParser p;

        for (auto const &entry : arr.values) {
            // Skip strings, those are just comments
            if (std::holds_alternative<std::string>(entry)) {
                continue;
            }

            auto const &obj = std::get<json::Object>(entry);
            bool should_fail = false;

            // Check if test expects failure
            if (obj.contains("failure")) {
                should_fail = true;
            }

            // Get input URL
            std::string_view input = std::get<std::string>(obj.at("input"));

            // Parse base URL if it exists
            std::optional<url::Url> base_test;

            if (auto const *base_str = std::get_if<std::string>(&obj.at("base"))) {
                base_test = p.parse(std::string{*base_str});
                a.require(base_test.has_value(), std::format("Parsing base URL:({}) failed", *base_str));
            }

            // Parse input URL
            std::optional<url::Url> url = p.parse(std::string{input}, base_test);

            if (!should_fail) {
                a.expect(url.has_value(), "Parsing input URL:(" + std::string{input} + ") failed");

                if (!url.has_value()) {
                    continue;
                }
            } else {
                a.require(!url.has_value(),
                        "Parsing input URL:(" + std::string{input} + ") succeeded when it was supposed to fail");

                // If this test was an expected failure, test ends here
                continue;
            }

            // Check URL fields against test

            std::string_view href = std::get<std::string>(obj.at("href"));
            a.expect_eq(url->serialize(), href);

            if (obj.contains("origin")) {
                std::string_view origin = std::get<std::string>(obj.at("origin"));

                a.expect_eq(url->origin().serialize(), origin);
            }

            std::string_view protocol = std::get<std::string>(obj.at("protocol"));
            a.expect_eq(url->scheme + ":", protocol);

            std::string_view username = std::get<std::string>(obj.at("username"));
            a.expect_eq(url->user, username);

            std::string_view password = std::get<std::string>(obj.at("password"));
            a.expect_eq(url->passwd, password);

            std::string_view hostname = std::get<std::string>(obj.at("hostname"));
            a.expect_eq(url->host.has_value() ? url->host->serialize() : "", hostname);

            std::string_view host = std::get<std::string>(obj.at("host"));
            std::string host_serialized = url->host.has_value() ? url->host->serialize() : "";
            std::string host_port = url->port.has_value() ? std::string{":"} + std::to_string(*url->port) : "";
            a.expect_eq(host_serialized + host_port, host);

            std::string_view port = std::get<std::string>(obj.at("port"));
            a.expect_eq(url->port.has_value() ? std::to_string(*url->port) : "", port);

            std::string_view pathname = std::get<std::string>(obj.at("pathname"));
            a.expect_eq(url->serialize_path(), pathname);

            std::string_view search = std::get<std::string>(obj.at("search"));
            a.expect_eq(url->query.has_value() && !url->query->empty() ? std::string{"?"} + *url->query : "", search);

            std::string_view hash = std::get<std::string>(obj.at("hash"));
            a.expect_eq(url->fragment.has_value() && !url->fragment->empty() ? std::string{"#"} + *url->fragment : "",
                    hash);
        }
    });

    int ret = s.run();

    url::icu_cleanup();

    return ret;
}
