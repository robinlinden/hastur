// SPDX-FileCopyrightText: 2022-2023 David Zero <zero-one@zer0-one.net>
// SPDX-FileCopyrightText: 2021-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef URL_URL_H_
#define URL_URL_H_

#include "util/string.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

namespace url {

void icu_cleanup();

enum class HostType : std::uint8_t { DnsDomain, Ip4Addr, Ip6Addr, Opaque, Empty };

struct Host {
    HostType type;

    std::variant<std::string, std::uint32_t, std::array<std::uint16_t, 8>> data;

    std::string serialize() const;

    bool operator==(Host const &) const = default;
};

struct Origin {
    std::string scheme;
    Host host;
    std::optional<std::uint16_t> port;
    std::optional<std::string> domain;
    // If opaque, then this Origin should serialize to (null). All opaque origins are equal to each other, and not equal
    // to all non-opaque origins.
    bool opaque = false;

    std::string serialize() const;
    std::variant<std::monostate, std::string, Host> effective_domain() const;

    // https://html.spec.whatwg.org/multipage/browsers.html#same-origin
    bool operator==(Origin const &b) const {
        if (opaque && b.opaque) {
            return true;
        }

        if (!opaque && !b.opaque) {
            if (scheme == b.scheme && host == b.host && port == b.port) {
                return true;
            }
        }

        return false;
    }

    // https://html.spec.whatwg.org/multipage/browsers.html#same-origin-domain
    constexpr bool is_same_origin_domain(Origin const &b) const {
        if (opaque && b.opaque) {
            return true;
        }

        if (!opaque && !b.opaque) {
            if (scheme == b.scheme && domain == b.domain && domain.has_value() && b.domain.has_value()) {
                return true;
            }

            if (*this == b && domain == b.domain && !domain.has_value() && !b.domain.has_value()) {
                return true;
            }
        }

        return false;
    }
};

/**
 * Generates a new Blob URL for the given origin.
 */
std::string blob_url_create(Origin const &origin);

struct Url {
    std::string scheme;
    std::string user;
    std::string passwd;
    std::optional<Host> host;
    std::optional<std::uint16_t> port;
    std::variant<std::string, std::vector<std::string>> path;
    std::optional<std::string> query;
    std::optional<std::string> fragment;

    std::string serialize(bool exclude_fragment = false, bool rfc3986_norm = false) const;
    std::string serialize_path() const;

    Origin origin() const;

    constexpr bool includes_credentials() const { return !user.empty() || !passwd.empty(); }
    constexpr bool has_opaque_path() const { return std::holds_alternative<std::string>(path); }

    // https://url.spec.whatwg.org/#url-equivalence
    bool operator==(Url const &b) const { return serialize() == b.serialize(); }
};

inline std::string to_string(url::Url const &url) {
    return url.serialize();
}

enum class ValidationError : std::uint8_t {
    // IDNA
    DomainToAscii,
    DomainToUnicode,
    // Host parsing
    DomainInvalidCodePoint,
    HostInvalidCodePoint,
    IPv4EmptyPart,
    IPv4TooManyParts,
    IPv4NonNumericPart,
    IPv4NonDecimalPart,
    IPv4OutOfRangePart,
    IPv6Unclosed,
    IPv6InvalidCompression,
    IPv6TooManyPieces,
    IPv6MultipleCompression,
    IPv6InvalidCodePoint,
    IPv6TooFewPieces,
    IPv4InIPv6TooManyPieces,
    IPv4InIPv6InvalidCodePoint,
    IPv4InIPv6OutOfRangePart,
    IPv4InIPv6TooFewParts,
    // URL parsing
    InvalidUrlUnit,
    SpecialSchemeMissingFollowingSolidus,
    MissingSchemeNonRelativeUrl,
    InvalidReverseSolidus,
    InvalidCredentials,
    HostMissing,
    PortOutOfRange,
    PortInvalid,
    FileInvalidWindowsDriveLetter,
    FileInvalidWindowsDriveLetterHost
};

constexpr std::string_view to_string(ValidationError e) {
    switch (e) {
        case ValidationError::DomainToAscii:
            return "DomainToAscii";
        case ValidationError::DomainToUnicode:
            return "DomainToUnicode";
        case ValidationError::DomainInvalidCodePoint:
            return "DomainInvalidCodePoint";
        case ValidationError::HostInvalidCodePoint:
            return "HostInvalidCodePoint";
        case ValidationError::IPv4EmptyPart:
            return "IPv4EmptyPart";
        case ValidationError::IPv4TooManyParts:
            return "IPv4TooManyParts";
        case ValidationError::IPv4NonNumericPart:
            return "IPv4NonNumericPart";
        case ValidationError::IPv4NonDecimalPart:
            return "IPv4NonDecimalPart";
        case ValidationError::IPv4OutOfRangePart:
            return "IPv4OutOfRangePart";
        case ValidationError::IPv6Unclosed:
            return "IPv6Unclosed";
        case ValidationError::IPv6InvalidCompression:
            return "IPv6InvalidCompression";
        case ValidationError::IPv6TooManyPieces:
            return "IPv6TooManyPieces";
        case ValidationError::IPv6MultipleCompression:
            return "IPv6MultipleCompression";
        case ValidationError::IPv6InvalidCodePoint:
            return "IPv6InvalidCodePoint";
        case ValidationError::IPv6TooFewPieces:
            return "IPv6TooFewPieces";
        case ValidationError::IPv4InIPv6TooManyPieces:
            return "IPv4InIPv6TooManyPieces";
        case ValidationError::IPv4InIPv6InvalidCodePoint:
            return "IPv4InIPv6InvalidCodePoint";
        case ValidationError::IPv4InIPv6OutOfRangePart:
            return "IPv4InIPv6OutOfRangePart";
        case ValidationError::IPv4InIPv6TooFewParts:
            return "IPv4InIPv6TooFewParts";
        case ValidationError::InvalidUrlUnit:
            return "InvalidUrlUnit";
        case ValidationError::SpecialSchemeMissingFollowingSolidus:
            return "SpecialSchemeMissingFollowingSolidus";
        case ValidationError::MissingSchemeNonRelativeUrl:
            return "MissingSchemeNonRelativeUrl";
        case ValidationError::InvalidReverseSolidus:
            return "InvalidReverseSolidus";
        case ValidationError::InvalidCredentials:
            return "InvalidCredentials";
        case ValidationError::HostMissing:
            return "HostMissing";
        case ValidationError::PortOutOfRange:
            return "PortOutOfRange";
        case ValidationError::PortInvalid:
            return "PortInvalid";
        case ValidationError::FileInvalidWindowsDriveLetter:
            return "FileInvalidWindowsDriveLetter";
        case ValidationError::FileInvalidWindowsDriveLetterHost:
            return "FileInvalidWindowsDriveLetterHost";
    }

    return "Unknown error";
}

std::string_view description(ValidationError);

// This parser is current with the WHATWG URL specification as of 27 September 2023
class UrlParser final {
public:
    std::optional<Url> parse(std::string input, std::optional<Url> base = std::nullopt);

    void set_on_error(std::function<void(ValidationError)> on_error) { on_error_ = std::move(on_error); }

private:
    enum class ParserState : std::uint8_t {
        SchemeStart,
        Scheme,
        NoScheme,
        SpecialRelativeOrAuthority,
        PathOrAuthority,
        Relative,
        RelativeSlash,
        SpecialAuthoritySlashes,
        SpecialAuthorityIgnoreSlashes,
        Authority,
        Host,
        Hostname,
        Port,
        File,
        FileSlash,
        FileHost,
        PathStart,
        Path,
        OpaquePath,
        Query,
        Fragment,
        Failure,
        Terminate
    };

    // Parse helpers
    constexpr std::optional<char> peek() const {
        if (is_eof()) {
            return std::nullopt;
        }

        return input_[pos_];
    }

    constexpr std::optional<std::string_view> peek(std::size_t chars) const {
        if (is_eof()) {
            return std::nullopt;
        }

        return input_.substr(pos_, chars);
    }

    constexpr std::string_view remaining_from(std::size_t skip) const {
        return pos_ + skip >= input_.size() ? "" : input_.substr(pos_ + skip);
    }

    constexpr bool is_eof() const { return pos_ >= input_.size(); }

    constexpr void advance(std::size_t n) { pos_ += n; }

    constexpr void back(std::size_t n) { pos_ -= n; }

    constexpr void reset() { pos_ = 0; }

    constexpr void reset(std::string_view input) {
        input_ = input;
        pos_ = 0;
    }

    // Main parser
    std::optional<Url> parse_basic(std::string input,
            std::optional<Url> base,
            std::optional<Url> url,
            std::optional<ParserState> state_override);

    void state_scheme_start();
    void state_scheme();
    void state_no_scheme();
    void state_special_relative_or_authority();
    void state_path_or_authority();
    void state_relative();
    void state_relative_slash();
    void state_special_authority_slashes();
    void state_special_authority_ignore_slashes();
    void state_authority();
    void state_host();
    void state_port();
    void state_file();
    void state_file_slash();
    void state_file_host();
    void state_path_start();
    void state_path();
    void state_opaque_path();
    void state_query();
    void state_fragment();

    void validation_error(ValidationError) const;

    // Host parsing
    std::optional<Host> parse_host(std::string_view input, bool is_not_special = false) const;
    bool ends_in_number(std::string_view) const;
    std::optional<std::uint32_t> parse_ipv4(std::string_view) const;
    std::optional<std::tuple<std::uint64_t, bool>> parse_ipv4_number(std::string_view) const;
    std::optional<std::array<std::uint16_t, 8>> parse_ipv6(std::string_view) const;
    std::optional<std::string> parse_opaque_host(std::string_view) const;
    bool is_url_codepoint(std::uint32_t) const;

    // IDNA
    std::optional<std::string> domain_to_ascii(std::string_view domain, bool be_strict) const;

    // Misc
    bool starts_with_windows_drive_letter(std::string_view) const;
    void shorten_url_path(Url &) const;

    constexpr bool is_windows_drive_letter(std::string_view input) const {
        return input.size() == 2 && util::is_alpha(input[0]) && (input[1] == ':' || input[1] == '|');
    }

    constexpr bool is_normal_windows_drive_letter(std::string_view input) const {
        return input.size() == 2 && util::is_alpha(input[0]) && input[1] == ':';
    }

    // Parser state
    std::string_view input_{};
    std::size_t pos_{0};

    Url url_;
    std::optional<Url> base_;
    std::optional<ParserState> state_override_;

    ParserState state_ = ParserState::Failure;

    std::string buffer_;

    bool at_sign_seen_ = false;
    bool inside_brackets_ = false;
    bool password_token_seen_ = false;

    std::function<void(ValidationError)> on_error_;
};

} // namespace url

#endif
