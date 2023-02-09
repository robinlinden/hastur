// SPDX-FileCopyrightText: 2023 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef URL_URL_H_
#define URL_URL_H_

#include "util/base_parser.h"
#include "util/string.h"

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace url {

void icu_cleanup();

enum class HostType { DnsDomain, Ip4Addr, Ip6Addr, Opaque, Empty };

struct Host {
    HostType type;

    std::variant<std::string, std::uint32_t, std::array<std::uint16_t, 8>> data;
};

struct Origin {
    std::string scheme;
    Host host;
    std::optional<std::uint16_t> port;
    std::optional<std::string> domain;
    // Need this placeholder until I figure out what "opaqueness" means for an origin in this context
    bool opaque;
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
};

// This parser is current with the WHATWG URL specification as of 1 March 2023
class UrlParser final : util::BaseParser {
public:
    UrlParser() : BaseParser{""} {}

    std::optional<Url> parse(std::string input, std::optional<Url> base = std::nullopt);

    enum class ValidationError {
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

private:
    enum class ParserState {
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

    constexpr bool includes_credentials(Url &url) const { return !url.user.empty() || !url.user.empty(); }

    constexpr bool has_opaque_path(Url &url) const { return std::holds_alternative<std::string>(url.path); }

    constexpr bool is_windows_drive_letter(std::string_view input) const {
        return input.size() == 2 && util::is_alpha(input[0]) && (input[1] == ':' || input[1] == '|');
    }

    constexpr bool is_normal_windows_drive_letter(std::string_view input) const {
        return input.size() == 2 && util::is_alpha(input[0]) && input[1] == ':';
    }

    // Parser state
    Url url_;
    std::optional<Url> base_;
    std::optional<ParserState> state_override_;

    ParserState state_ = ParserState::Failure;

    std::string buffer_;

    bool at_sign_seen_ = false;
    bool inside_brackets_ = false;
    bool password_token_seen_ = false;
};

} // namespace url

#endif
