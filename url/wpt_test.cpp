// SPDX-FileCopyrightText: 2023 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2023-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "url/url.h"

#include "etest/etest2.h"

#include <simdjson.h> // IWYU pragma: keep

#include <iostream>
#include <optional>
#include <string>
#include <string_view>

int main(int argc, char **argv) {
    if (argc != 2) {
        auto exe = std::string_view{argv[0] != nullptr ? argv[0] : "<executable>"};
        std::cerr << "Usage: " << exe << " <path to urltestdata.json>\n";
        return 1;
    }

    char const *urltestdata = argv[1];

    etest::Suite s{};

    // NOLINTBEGIN(misc-include-cleaner): What you're meant to include from
    // simdjson depends on things like the architecture you're compiling for.
    // This is handled automagically with detection macros inside simdjson.
    s.add_test("Web Platform Tests", [&](etest::IActions &a) {
        url::UrlParser p;

        simdjson::ondemand::parser parser;

        // NOLINTNEXTLINE(clang-analyzer-unix.Errno): Problem in simdjson that probably doesn't affect us.
        auto json = simdjson::padded_string::load(urltestdata);

        simdjson::ondemand::document doc = parser.iterate(json);

        simdjson::ondemand::array arr = doc.get_array();

        for (auto obj : arr) {
            // Skip strings, those are just comments
            if (obj.type() == simdjson::ondemand::json_type::string) {
                continue;
            }

            bool should_fail = false;

            // Check if test expects failure
            if (obj.find_field("failure").error() != simdjson::error_code::NO_SUCH_FIELD) {
                should_fail = true;
            }

            // Get input URL
            std::string_view input = obj["input"].get_string();

            // Parse base URL if it exists
            std::optional<url::Url> base_test;

            if (!obj["base"].is_null()) {
                std::string_view base_str = obj["base"].get_string();

                base_test = p.parse(std::string{base_str});

                if (!should_fail) {
                    a.expect(base_test.has_value(), "Parsing base URL:(" + std::string{base_str} + ") failed");

                    continue;
                }
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

            std::string_view href = obj["href"];
            a.expect_eq(url->serialize(), href);

            if (obj.find_field("origin").error() != simdjson::error_code::NO_SUCH_FIELD) {
                std::string_view origin = obj["origin"];

                a.expect_eq(url->origin().serialize(), origin);
            }

            std::string_view protocol = obj["protocol"];
            a.expect_eq(url->scheme + ":", protocol);

            std::string_view username = obj["username"];
            a.expect_eq(url->user, username);

            std::string_view password = obj["password"];
            a.expect_eq(url->passwd, password);

            std::string_view hostname = obj["hostname"];
            a.expect_eq(url->host.has_value() ? url->host->serialize() : "", hostname);

            std::string_view host = obj["host"];
            std::string host_serialized = url->host.has_value() ? url->host->serialize() : "";
            std::string host_port = url->port.has_value() ? std::string{":"} + std::to_string(*url->port) : "";
            a.expect_eq(host_serialized + host_port, host);

            std::string_view port = obj["port"];
            a.expect_eq(url->port.has_value() ? std::to_string(*url->port) : "", port);

            std::string_view pathname = obj["pathname"];
            a.expect_eq(url->serialize_path(), pathname);

            std::string_view search = obj["search"];
            a.expect_eq(url->query.has_value() && !url->query->empty() ? std::string{"?"} + *url->query : "", search);

            std::string_view hash = obj["hash"];
            a.expect_eq(url->fragment.has_value() && !url->fragment->empty() ? std::string{"#"} + *url->fragment : "",
                    hash);
        }
    });
    // NOLINTEND(misc-include-cleaner)

    int ret = s.run();

    url::icu_cleanup();

    return ret;
}
