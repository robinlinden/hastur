// SPDX-FileCopyrightText: 2023 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "url/url.h"

#include "url/rtti_hack.h"

#include "util/string.h"
#include "util/unicode.h"
#include "util/uuid.h"

#include <spdlog/spdlog.h>
#include <unicode/bytestream.h>
#include <unicode/idna.h>
#include <unicode/putil.h>
#include <unicode/uclean.h>

#include <array>
#include <atomic>
#include <cassert>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

namespace url {
// NOLINTBEGIN(misc-redundant-expression)
// NOLINTBEGIN(bugprone-unchecked-optional-access)

namespace {
// NOLINTNEXTLINE(cert-err58-cpp)
std::map<std::string, std::uint16_t> const special_schemes = {{"ftp", std::uint16_t{21}},
        {"file", std::uint16_t{0}},
        {"http", std::uint16_t{80}},
        {"https", std::uint16_t{443}},
        {"ws", std::uint16_t{80}},
        {"wss", std::uint16_t{443}}};

// NOLINTNEXTLINE(cert-err58-cpp)
std::map<UrlParser::ValidationError, std::string> const validation_error_str = {
        {UrlParser::ValidationError::DomainToAscii, "Unicode ToASCII records an error or returns the empty string"},
        {UrlParser::ValidationError::DomainToUnicode, "Unicode ToUnicode records an error"},
        {UrlParser::ValidationError::DomainInvalidCodePoint, "The input's host contains a forbidden domain code point"},
        {UrlParser::ValidationError::HostInvalidCodePoint,
                "An opaque host (in a URL that is not special) contains a forbidden host code point"},
        {UrlParser::ValidationError::IPv4EmptyPart, "An IPv4 address ends with a U+002E (.)"},
        {UrlParser::ValidationError::IPv4TooManyParts, "An IPv4 address does not consist of exactly 4 parts"},
        {UrlParser::ValidationError::IPv4NonNumericPart, "An IPv4 address part is not numeric"},
        {UrlParser::ValidationError::IPv4NonDecimalPart,
                "The IPv4 address contains numbers expressed using hexadecimal or octal digits"},
        {UrlParser::ValidationError::IPv4OutOfRangePart, "An IPv4 address part exceeds 255"},
        {UrlParser::ValidationError::IPv6Unclosed, "An IPv6 address is missing the closing U+005D (])"},
        {UrlParser::ValidationError::IPv6InvalidCompression, "An IPv6 address begins with improper compression"},
        {UrlParser::ValidationError::IPv6TooManyPieces, "An IPv6 address contains more than 8 pieces"},
        {UrlParser::ValidationError::IPv6MultipleCompression, "An IPv6 address is compressed in more than one spot"},
        {UrlParser::ValidationError::IPv6InvalidCodePoint,
                "An IPv6 address contains a code point that is neither an ASCII hex digit nor a U+003A (:), or it "
                "unexpectedly ends"},
        {UrlParser::ValidationError::IPv6TooFewPieces, "An uncompressed IPv6 address contains fewer than 8 pieces"},
        {UrlParser::ValidationError::IPv4InIPv6TooManyPieces,
                "An IPv6 address with IPv4 address syntax: the IPv6 address has more than 6 pieces"},
        {UrlParser::ValidationError::IPv4InIPv6InvalidCodePoint,
                "An IPv6 address with IPv4 address syntax: An IPv4 part is empty or contains a non-ASCII digit, an "
                "IPv4 part contains a leading 0, or there are too many IPv4 parts"},
        {UrlParser::ValidationError::IPv4InIPv6OutOfRangePart,
                "An IPv6 address with IPv4 address syntax: an IPv4 part exceeds 255"},
        {UrlParser::ValidationError::IPv4InIPv6TooFewParts,
                "An IPv6 address with IPv4 address syntax: an IPv4 address contains too few parts"},
        {UrlParser::ValidationError::InvalidUrlUnit, "A code point is found that is not a URL unit"},
        {UrlParser::ValidationError::SpecialSchemeMissingFollowingSolidus,
                "The input's scheme is not followed by \"//\""},
        {UrlParser::ValidationError::MissingSchemeNonRelativeUrl,
                "The input is missing a scheme, because it does not begin with an ASCII alpha, and either no base "
                "URL was provided or the base URL cannot be used as a base URL because it has an opaque path"},
        {UrlParser::ValidationError::InvalidReverseSolidus,
                "The URL has a special scheme and it uses U+005C (\\) instead of U+002F (/)"},
        {UrlParser::ValidationError::InvalidCredentials, "The input includes credentials"},
        {UrlParser::ValidationError::HostMissing, "The input has a special scheme, but does not contain a host"},
        {UrlParser::ValidationError::PortOutOfRange, "The input's port is too big"},
        {UrlParser::ValidationError::PortInvalid, "The input's port is invalid"},
        {UrlParser::ValidationError::FileInvalidWindowsDriveLetter,
                "The input is a relative-URL string that starts with a Windows drive letter and the base URL's "
                "scheme is \"file\""},
        {UrlParser::ValidationError::FileInvalidWindowsDriveLetterHost,
                "A file: URL's host is a Windows drive letter"}};

struct PercentEncodeSet {
    static constexpr bool c0_control(char c) {
        return util::is_c0(c) || c == 0x7f || static_cast<std::uint8_t>(c) > 0x7f;
    }

    static constexpr bool fragment(char c) {
        return c0_control(c) || c == ' ' || c == '"' || c == '<' || c == '>' || c == '`';
    }

    static constexpr bool query(char c) {
        return c0_control(c) || c == ' ' || c == '"' || c == '#' || c == '<' || c == '>';
    }

    static constexpr bool special_query(char c) { return query(c) || c == '\''; }

    static constexpr bool path(char c) { return query(c) || c == '?' || c == '`' || c == '{' || c == '}'; }

    static constexpr bool userinfo(char c) {
        return path(c) || c == '/' || c == ':' || c == ';' || c == '=' || c == '@' || (c >= '[' && c <= '^')
                || c == '|';
    }

    static constexpr bool component(char c) { return userinfo(c) || (c >= '$' && c <= '&') || c == '+' || c == ','; }
};

} // namespace

void icu_cleanup() {
    u_cleanup();
}

// https://html.spec.whatwg.org/multipage/browsers.html#ascii-serialisation-of-an-origin
std::string Origin::serialize() const {
    if (opaque) {
        return "null";
    }

    std::string result = scheme;

    result += "://";

    result += host.serialize();

    if (port.has_value()) {
        result += ":";

        result += std::to_string(*port);
    }

    return result;
}

// https://html.spec.whatwg.org/multipage/browsers.html#concept-origin-effective-domain
std::variant<std::monostate, std::string, Host> Origin::effective_domain() const {
    if (opaque) {
        return std::monostate{};
    }

    if (domain.has_value()) {
        return *domain;
    }

    return host;
}

// https://w3c.github.io/FileAPI/#unicodeBlobURL
std::string blob_url_create(Origin const &origin) {
    std::string result = "blob:";
    std::string serialized;

    // https://html.spec.whatwg.org/multipage/browsers.html#ascii-serialisation-of-an-origin
    if (origin.opaque) {
        serialized = "null";
    } else {
        serialized = origin.scheme + "://";

        switch (origin.host.type) {
            case HostType::DnsDomain:
            case HostType::Opaque:
            case HostType::Empty:
                serialized += std::get<std::string>(origin.host.data);
                break;
            case HostType::Ip4Addr:
                serialized += util::ipv4_serialize(std::get<std::uint32_t>(origin.host.data));
                break;
            case HostType::Ip6Addr:
                std::array<std::uint16_t, 8> v6 = std::get<std::array<std::uint16_t, 8>>(origin.host.data);
                serialized += "[" + util::ipv6_serialize(v6) + "]";
        }

        if (origin.port.has_value()) {
            serialized += ":" + std::to_string(*origin.port);
        }
    }

    result += serialized;
    result += "/";
    result += util::new_uuid();

    return result;
}

// https://url.spec.whatwg.org/#concept-host-serializer
std::string Host::serialize() const {
    if (type == HostType::Ip4Addr) {
        return util::ipv4_serialize(std::get<std::uint32_t>(data));
    } else if (type == HostType::Ip6Addr) {
        return "[" + util::ipv6_serialize(std::get<2>(data)) + "]";
    }

    return std::get<std::string>(data);
}

// https://url.spec.whatwg.org/#url-path-serializer
std::string Url::serialize_path() const {
    if (has_opaque_path()) {
        return std::get<0>(path);
    }

    std::string output;

    for (auto const &part : std::get<1>(path)) {
        output += "/" + part;
    }

    return output;
}

// https://url.spec.whatwg.org/#concept-url-serializer
std::string Url::serialize(bool exclude_fragment) const {
    std::string output = scheme + ":";

    if (host.has_value()) {
        output += "//";

        if (includes_credentials()) {
            output += user;

            if (!passwd.empty()) {
                output += ":" + passwd;
            }

            output += "@";
        }

        output += host->serialize();

        if (port.has_value()) {
            output += ":" + std::to_string(*port);
        }
    }

    if (!host.has_value() && std::holds_alternative<std::vector<std::string>>(path) && std::get<1>(path).size() > 1
            && std::get<1>(path)[0].empty()) {
        output += "/.";
    }

    output += serialize_path();

    if (query.has_value()) {
        output += "?" + *query;
    }

    if (!exclude_fragment && fragment.has_value()) {
        output += "#" + *fragment;
    }

    return output;
}

// https://url.spec.whatwg.org/#concept-url-origin
Origin Url::origin() const {
    // Return tuple origin of the path URL
    if (scheme == "blob") {
        // TODO(dzero): Implement checking blob URL entry, once those are implemented
        UrlParser p;

        std::optional<Url> path_url = p.parse(serialize_path());

        if (!path_url.has_value()) {
            return Origin{"", Host{}, std::nullopt, std::nullopt, true};
        }

        if (path_url->scheme != "http" && path_url->scheme != "https") {
            return Origin{"", Host{}, std::nullopt, std::nullopt, true};
        }

        return path_url->origin();
    }
    // Return a tuple origin
    else if (scheme == "ftp" || scheme == "http" || scheme == "https" || scheme == "ws" || scheme == "wss") {
        // These schemes all require a host in a valid URL
        assert(host.has_value());

        return Origin{scheme, *host, port, std::nullopt};
    }
    // Return a new opaque origin
    else {
        return Origin{"", Host{}, std::nullopt, std::nullopt, true};
    }
}

void UrlParser::validation_error(ValidationError err) const {
    spdlog::warn("url: InputPos: {}, ParserState: {}, Validation Error: {} {}",
            current_pos(),
            std::to_underlying(state_),
            std::to_underlying(err),
            validation_error_str.at(err));
}

// https://url.spec.whatwg.org/#concept-url-parser
std::optional<Url> UrlParser::parse(std::string input, std::optional<Url> base) {
    if (input.empty() && !base.has_value()) {
        return std::nullopt;
    }

    std::optional<Url> url = parse_basic(std::move(input), std::move(base), std::nullopt, std::nullopt);

    if (url.has_value() && url->scheme == "blob") {
        // TODO(zero-one): Resolve blob URL
    }

    return url;
}

// https://url.spec.whatwg.org/#concept-basic-url-parser
std::optional<Url> UrlParser::parse_basic(std::string input,
        std::optional<Url> base, // NOLINT(bugprone-easily-swappable-parameters)
        std::optional<Url> url, // NOLINT(bugprone-easily-swappable-parameters)
        std::optional<ParserState> state_override) {
    base_ = std::move(base);
    state_override_ = state_override;

    if (!url.has_value()) {
        // Set url to a new URL
        url_ = Url();
        url_.path = std::vector<std::string>{};

        bool leading_trailing_c0 = false;

        while (!input.empty() && util::is_c0_or_space(input.front())) {
            input.erase(0, 1);

            leading_trailing_c0 = true;
        }

        while (!input.empty() && util::is_c0_or_space(input.back())) {
            input.pop_back();

            leading_trailing_c0 = true;
        }

        if (leading_trailing_c0) {
            validation_error(ValidationError::InvalidUrlUnit);
        }
    } else {
        url_ = *url;
    }

    if (std::erase_if(input, util::is_tab_or_newline) > 0) {
        validation_error(ValidationError::InvalidUrlUnit);
    }

    state_ = state_override_.value_or(ParserState::SchemeStart);

    buffer_.clear();

    at_sign_seen_ = false;
    inside_brackets_ = false;
    password_token_seen_ = false;

    // Initialize BaseParser with our modified input
    reset(input);

    while (true) {
        switch (state_) {
            case ParserState::SchemeStart:
                state_scheme_start();
                break;
            case ParserState::Scheme:
                state_scheme();
                break;
            case ParserState::NoScheme:
                state_no_scheme();
                break;
            case ParserState::SpecialRelativeOrAuthority:
                state_special_relative_or_authority();
                break;
            case ParserState::PathOrAuthority:
                state_path_or_authority();
                break;
            case ParserState::Relative:
                state_relative();
                break;
            case ParserState::RelativeSlash:
                state_relative_slash();
                break;
            case ParserState::SpecialAuthoritySlashes:
                state_special_authority_slashes();
                break;
            case ParserState::SpecialAuthorityIgnoreSlashes:
                state_special_authority_ignore_slashes();
                break;
            case ParserState::Authority:
                state_authority();
                break;
            case ParserState::Host:
            case ParserState::Hostname:
                state_host();
                break;
            case ParserState::Port:
                state_port();
                break;
            case ParserState::File:
                state_file();
                break;
            case ParserState::FileSlash:
                state_file_slash();
                break;
            case ParserState::FileHost:
                state_file_host();
                break;
            case ParserState::PathStart:
                state_path_start();
                break;
            case ParserState::Path:
                state_path();
                break;
            case ParserState::OpaquePath:
                state_opaque_path();
                break;
            case ParserState::Query:
                state_query();
                break;
            case ParserState::Fragment:
                state_fragment();
                break;
            case ParserState::Failure:
                return std::nullopt;
            case ParserState::Terminate:
                // I use this state where the spec returns "nothing" (i.e, the parser is modifying a given optional URL)
                // Instead of modifying it in-place, I modify a copy and return that instead of nothing.
                return url_;
        }

        // This check accomodates the one scenario (commented on in
        // state_scheme_start, below) in which the parser position goes
        // negative.
        if (is_eof() && current_pos() != static_cast<std::size_t>(-1)) {
            break;
        }

        advance(1);
    }

    return url_;
}

// https://url.spec.whatwg.org/#scheme-start-state
void UrlParser::state_scheme_start() {
    if (auto c = peek(); c.has_value() && util::is_alpha(*c)) {
        buffer_ += util::lowercased(*c);

        state_ = ParserState::Scheme;
    } else if (!state_override_.has_value()) {
        state_ = ParserState::NoScheme;

        // This can underflow pos_; that's ok, because it's incremented again before it's ever used.
        back(1);
    } else {
        state_ = ParserState::Failure;

        return;
    }
}

// https://url.spec.whatwg.org/#scheme-state
void UrlParser::state_scheme() {
    if (auto c = peek(); c.has_value() && (util::is_alphanumeric(*c) || c == '+' || c == '-' || c == '.')) {
        buffer_ += util::lowercased(*c);
    } else if (c == ':') {
        if (state_override_.has_value()) {
            if (special_schemes.contains(url_.scheme) && !special_schemes.contains(buffer_)) {
                state_ = ParserState::Terminate;

                return;
            }
            if (!special_schemes.contains(url_.scheme) && special_schemes.contains(buffer_)) {
                state_ = ParserState::Terminate;

                return;
            }
            if ((url_.includes_credentials() || url_.port.has_value()) && buffer_ == "file") {
                state_ = ParserState::Terminate;

                return;
            }
            if (url_.scheme == "file" && url_.host.has_value() && url_.host->type == HostType::Empty) {
                state_ = ParserState::Terminate;

                return;
            }
        }

        url_.scheme = buffer_;

        if (state_override_.has_value()) {
            if (special_schemes.contains(url_.scheme) && url_.port == special_schemes.at(url_.scheme)) {
                url_.port.reset();
            }

            state_ = ParserState::Terminate;

            return;
        }

        buffer_.clear();

        if (url_.scheme == "file") {
            if (!remaining_from(1).starts_with("//")) {
                validation_error(ValidationError::SpecialSchemeMissingFollowingSolidus);
            }

            state_ = ParserState::File;
        } else if (special_schemes.contains(url_.scheme) && base_.has_value() && base_->scheme == url_.scheme) {
            assert(special_schemes.contains(base_->scheme));

            state_ = ParserState::SpecialRelativeOrAuthority;
        } else if (special_schemes.contains(url_.scheme)) {
            state_ = ParserState::SpecialAuthoritySlashes;
        } else if (remaining_from(1).starts_with('/')) {
            state_ = ParserState::PathOrAuthority;

            advance(1);
        } else {
            url_.path = "";

            state_ = ParserState::OpaquePath;
        }
    } else if (!state_override_.has_value()) {
        buffer_.clear();

        state_ = ParserState::NoScheme;

        reset();

        // This can underflow pos_; that's ok, because it's incremented again before it's ever used.
        back(1);
    } else {
        state_ = ParserState::Failure;

        return;
    }
}

// https://url.spec.whatwg.org/#no-scheme-state
void UrlParser::state_no_scheme() {
    if (auto c = peek(); !base_.has_value() || (base_->has_opaque_path() && c != '#')) {
        validation_error(ValidationError::MissingSchemeNonRelativeUrl);

        state_ = ParserState::Failure;

        return;
    } else if (base_->has_opaque_path() && c == '#') {
        url_.scheme = base_->scheme;
        url_.path = base_->path;
        url_.query = base_->query;
        url_.fragment = "";

        state_ = ParserState::Fragment;
    } else if (base_->scheme != "file") {
        state_ = ParserState::Relative;

        back(1);
    } else {
        state_ = ParserState::File;

        back(1);
    }
}

// https://url.spec.whatwg.org/#special-relative-or-authority-state
void UrlParser::state_special_relative_or_authority() {
    if (peek() == '/' && remaining_from(1).starts_with('/')) {
        state_ = ParserState::SpecialAuthorityIgnoreSlashes;

        advance(1);
    } else {
        validation_error(ValidationError::SpecialSchemeMissingFollowingSolidus);

        state_ = ParserState::Relative;

        back(1);
    }
}

// https://url.spec.whatwg.org/#path-or-authority-state
void UrlParser::state_path_or_authority() {
    if (peek() == '/') {
        state_ = ParserState::Authority;
    } else {
        state_ = ParserState::Path;

        back(1);
    }
}

// https://url.spec.whatwg.org/#relative-state
void UrlParser::state_relative() {
    assert(base_.has_value() && base_->scheme != "file");

    url_.scheme = base_->scheme;

    if (auto c = peek(); c == '/') {
        state_ = ParserState::RelativeSlash;
    } else if (special_schemes.contains(url_.scheme) && c == '\\') {
        validation_error(ValidationError::InvalidReverseSolidus);

        state_ = ParserState::RelativeSlash;
    } else {
        url_.user = base_->user;
        url_.passwd = base_->passwd;
        url_.host = base_->host;
        url_.port = base_->port;
        url_.path = base_->path;
        url_.query = base_->query;

        if (c == '?') {
            url_.query = "";

            state_ = ParserState::Query;
        } else if (c == '#') {
            url_.fragment = "";

            state_ = ParserState::Fragment;
        } else if (!is_eof()) {
            url_.query.reset();

            shorten_url_path(url_);

            state_ = ParserState::Path;

            back(1);
        }
    }
}

// https://url.spec.whatwg.org/#relative-slash-state
void UrlParser::state_relative_slash() {
    if (auto c = peek(); special_schemes.contains(url_.scheme) && (c == '/' || c == '\\')) {
        if (c == '\\') {
            validation_error(ValidationError::InvalidReverseSolidus);
        }

        state_ = ParserState::SpecialAuthorityIgnoreSlashes;
    } else if (c == '/') {
        state_ = ParserState::Authority;
    } else {
        url_.user = base_->user;
        url_.passwd = base_->passwd;
        url_.host = base_->host;
        url_.port = base_->port;

        state_ = ParserState::Path;

        back(1);
    }
}

// https://url.spec.whatwg.org/#special-authority-slashes-state
void UrlParser::state_special_authority_slashes() {
    if (peek() == '/' && remaining_from(1).starts_with('/')) {
        state_ = ParserState::SpecialAuthorityIgnoreSlashes;

        advance(1);
    } else {
        validation_error(ValidationError::SpecialSchemeMissingFollowingSolidus);

        state_ = ParserState::SpecialAuthorityIgnoreSlashes;

        back(1);
    }
}

// https://url.spec.whatwg.org/#special-authority-ignore-slashes-state
void UrlParser::state_special_authority_ignore_slashes() {
    if (auto c = peek(); c != '/' && c != '\\') {
        state_ = ParserState::Authority;

        back(1);
    } else {
        validation_error(ValidationError::SpecialSchemeMissingFollowingSolidus);
    }
}

// https://url.spec.whatwg.org/#authority-state
void UrlParser::state_authority() {
    if (auto c = peek(); c == '@') {
        validation_error(ValidationError::InvalidCredentials);

        if (at_sign_seen_) {
            buffer_.insert(0, "%40");
        }

        at_sign_seen_ = true;

        for (std::size_t i = 0; i < buffer_.size(); i++) {
            if (buffer_[i] == ':' && !password_token_seen_) {
                password_token_seen_ = true;

                continue;
            }

            std::string encoded_code_points =
                    util::percent_encode(std::string_view{buffer_}.substr(i, 1), PercentEncodeSet::userinfo);

            if (password_token_seen_) {
                url_.passwd += encoded_code_points;
            } else {
                url_.user += encoded_code_points;
            }
        }

        buffer_.clear();
    } else if (is_eof() || c == '/' || c == '?' || c == '#' || (special_schemes.contains(url_.scheme) && c == '\\')) {
        if (at_sign_seen_ && buffer_.empty()) {
            validation_error(ValidationError::HostMissing);

            state_ = ParserState::Failure;

            return;
        }

        // The spec says to use code-point length, but that causes the parser
        // not to back up far enough; it will truncate characters going into
        // the host state.  It seems to only apply if you're parsing codepoint
        // by codepoint instead of byte-by-byte like we are.
        // back(util::utf8_length(buffer_) + 1);
        back(buffer_.size() + 1);

        buffer_.clear();

        state_ = ParserState::Host;
    } else {
        buffer_ += *c;
    }
}

// https://url.spec.whatwg.org/#host-state
void UrlParser::state_host() {
    if (auto c = peek(); state_override_.has_value() && url_.scheme == "file") {
        back(1);

        state_ = ParserState::FileHost;
    } else if (c == ':' && !inside_brackets_) {
        if (buffer_.empty()) {
            validation_error(ValidationError::HostMissing);

            state_ = ParserState::Failure;

            return;
        }

        if (state_override_.has_value() && *state_override_ == ParserState::Hostname) {
            state_ = ParserState::Terminate;

            return;
        }

        std::optional<Host> host = parse_host(buffer_, !special_schemes.contains(url_.scheme));

        if (!host.has_value()) {
            state_ = ParserState::Failure;

            return;
        }

        url_.host = host;

        buffer_.clear();

        state_ = ParserState::Port;
    } else if ((is_eof() || c == '/' || c == '?' || c == '#') || (special_schemes.contains(url_.scheme) && c == '\\')) {
        back(1);

        if (special_schemes.contains(url_.scheme) && buffer_.empty()) {
            validation_error(ValidationError::HostMissing);

            state_ = ParserState::Failure;

            return;
        } else if (state_override_.has_value() && buffer_.empty()
                && (url_.includes_credentials() || url_.port.has_value())) {
            state_ = ParserState::Terminate;

            return;
        }

        std::optional<Host> host = parse_host(buffer_, !special_schemes.contains(url_.scheme));

        if (!host.has_value()) {
            state_ = ParserState::Failure;

            return;
        }

        url_.host = host;

        buffer_.clear();

        state_ = ParserState::PathStart;

        if (state_override_.has_value()) {
            state_ = ParserState::Terminate;

            return;
        }
    } else {
        if (c == '[') {
            inside_brackets_ = true;
        }
        if (c == ']') {
            inside_brackets_ = false;
        }

        buffer_ += *c;
    }
}

// https://url.spec.whatwg.org/#port-state
void UrlParser::state_port() {
    if (auto c = peek(); c.has_value() && util::is_digit(*c)) {
        buffer_ += *c;
    } else if ((is_eof() || c == '/' || c == '?' || c == '#') || (special_schemes.contains(url_.scheme) && c == '\\')
            || state_override_.has_value()) {
        if (!buffer_.empty()) {
            std::uint32_t port;

            auto res = std::from_chars(buffer_.data(), buffer_.data() + buffer_.size(), port);

            if (res.ec == std::errc::invalid_argument || res.ec == std::errc::result_out_of_range) {
                spdlog::info("Invalid port given in URL");

                state_ = ParserState::Failure;

                return;
            }

            if (port > std::pow(2, 16) - 1) {
                validation_error(ValidationError::PortOutOfRange);

                state_ = ParserState::Failure;

                return;
            }

            if (special_schemes.contains(url_.scheme) && port == special_schemes.at(url_.scheme)) {
                url_.port = std::nullopt;
            } else {
                url_.port = static_cast<std::uint16_t>(port);
            }

            buffer_.clear();
        }
        if (state_override_.has_value()) {
            state_ = ParserState::Terminate;

            return;
        }

        state_ = ParserState::PathStart;

        back(1);
    } else {
        validation_error(ValidationError::PortInvalid);

        state_ = ParserState::Failure;

        return;
    }
}

// https://url.spec.whatwg.org/#file-state
void UrlParser::state_file() {
    url_.scheme = "file";
    url_.host = Host{HostType::Empty};

    if (auto c = peek(); c == '/' || c == '\\') {
        if (c == '\\') {
            validation_error(ValidationError::InvalidReverseSolidus);
        }

        state_ = ParserState::FileSlash;
    } else if (base_.has_value() && base_->scheme == "file") {
        url_.host = base_->host;
        url_.path = base_->path;
        url_.query = base_->query;

        if (c == '?') {
            url_.query = "";

            state_ = ParserState::Query;
        } else if (c == '#') {
            url_.fragment = "";

            state_ = ParserState::Fragment;
        } else if (!is_eof()) {
            url_.query = std::nullopt;

            if (!starts_with_windows_drive_letter(remaining_from(0))) {
                shorten_url_path(url_);
            } else {
                validation_error(ValidationError::FileInvalidWindowsDriveLetter);

                url_.path = std::vector<std::string>{};
            }

            state_ = ParserState::Path;

            back(1);
        }
    } else {
        state_ = ParserState::Path;

        back(1);
    }
}

// https://url.spec.whatwg.org/#file-slash-state
void UrlParser::state_file_slash() {
    if (auto c = peek(); c == '/' || c == '\\') {
        if (c == '\\') {
            validation_error(ValidationError::InvalidReverseSolidus);
        }

        state_ = ParserState::FileHost;
    } else {
        if (base_.has_value() && base_->scheme == "file") {
            url_.host = base_->host;

            if (!starts_with_windows_drive_letter(remaining_from(0))
                    && is_normal_windows_drive_letter(std::get<1>(base_->path)[0])) {
                std::get<1>(url_.path).push_back(std::get<1>(base_->path)[0]);
            }
        }

        state_ = ParserState::Path;

        back(1);
    }
}

// https://url.spec.whatwg.org/#file-host-state
void UrlParser::state_file_host() {
    if (auto c = peek(); is_eof() || c == '/' || c == '\\' || c == '?' || c == '#') {
        back(1);

        if (!state_override_.has_value() && is_windows_drive_letter(buffer_)) {
            validation_error(ValidationError::FileInvalidWindowsDriveLetterHost);

            state_ = ParserState::Path;
        } else if (buffer_.empty()) {
            url_.host = Host{HostType::Empty};

            if (state_override_.has_value()) {
                state_ = ParserState::Terminate;

                return;
            }

            state_ = ParserState::PathStart;
        } else {
            std::optional<Host> host = parse_host(buffer_, !special_schemes.contains(url_.scheme));

            if (!host.has_value()) {
                state_ = ParserState::Failure;

                return;
            }

            if (auto *h = std::get_if<0>(&host->data); h != nullptr && *h == "localhost") {
                *h = "";
            }

            url_.host = host;

            if (state_override_.has_value()) {
                state_ = ParserState::Terminate;

                return;
            }

            buffer_.clear();

            state_ = ParserState::PathStart;
        }
    } else {
        buffer_ += *c;
    }
}

// https://url.spec.whatwg.org/#path-start-state
void UrlParser::state_path_start() {
    if (auto c = peek(); special_schemes.contains(url_.scheme)) {
        if (c == '\\') {
            validation_error(ValidationError::InvalidReverseSolidus);
        }

        state_ = ParserState::Path;

        if (c != '/' && c != '\\') {
            back(1);
        }
    } else if (!state_override_.has_value() && c == '?') {
        url_.query = "";

        state_ = ParserState::Query;
    } else if (!state_override_.has_value() && c == '#') {
        url_.fragment = "";

        state_ = ParserState::Fragment;
    } else if (!is_eof()) {
        state_ = ParserState::Path;

        if (c != '/') {
            back(1);
        }
    } else if (state_override_.has_value() && !url_.host.has_value()) {
        std::get<1>(url_.path).push_back("");
    }
}

// https://url.spec.whatwg.org/#path-state
void UrlParser::state_path() {
    if (auto c = peek(); is_eof() || c == '/' || (special_schemes.contains(url_.scheme) && c == '\\')
            || (!state_override_.has_value() && (c == '?' || c == '#'))) {
        if (special_schemes.contains(url_.scheme) && c == '\\') {
            validation_error(ValidationError::InvalidReverseSolidus);
        }

        if (buffer_ == ".." || util::lowercased(buffer_) == ".%2e" || util::lowercased(buffer_) == "%2e."
                || util::lowercased(buffer_) == "%2e%2e") {
            shorten_url_path(url_);

            if (c != '/' && !(special_schemes.contains(url_.scheme) && c == '\\')) {
                std::get<1>(url_.path).push_back("");
            }
        } else if ((buffer_ == "." || util::lowercased(buffer_) == "%2e")
                && (c != '/' && !(special_schemes.contains(url_.scheme) && c == '\\'))) {
            std::get<1>(url_.path).push_back("");
        } else if (buffer_ != "." && util::lowercased(buffer_) != "%2e") {
            if (url_.scheme == "file" && std::get<1>(url_.path).empty() && is_windows_drive_letter(buffer_)) {
                buffer_[1] = ':';
            }

            std::get<1>(url_.path).push_back(buffer_);
        }

        buffer_.clear();

        if (c == '?') {
            url_.query = "";

            state_ = ParserState::Query;
        }

        if (c == '#') {
            url_.fragment = "";

            state_ = ParserState::Fragment;
        }
    } else {
        if (!is_url_codepoint(util::utf8_to_utf32(remaining_from(0))) && c != '%') {
            validation_error(ValidationError::InvalidUrlUnit);
        }

        if (c == '%'
                && (remaining_from(1).size() < 2 || !util::is_hex_digit(remaining_from(1)[0])
                        || !util::is_hex_digit(remaining_from(1)[1]))) {
            validation_error(ValidationError::InvalidUrlUnit);
        }

        buffer_ += util::percent_encode(*peek(1), PercentEncodeSet::path);
    }
}

// https://url.spec.whatwg.org/#cannot-be-a-base-url-path-state
void UrlParser::state_opaque_path() {
    if (auto c = peek(); c == '?') {
        url_.query = "";

        state_ = ParserState::Query;
    } else if (c == '#') {
        url_.fragment = "";

        state_ = ParserState::Fragment;
    } else {
        if (!is_eof() && !is_url_codepoint(util::utf8_to_utf32(remaining_from(0))) && c != '%') {
            validation_error(ValidationError::InvalidUrlUnit);
        }

        if (c == '%'
                && (remaining_from(1).size() < 2 || !util::is_hex_digit(remaining_from(1)[0])
                        || !util::is_hex_digit(remaining_from(1)[1]))) {
            validation_error(ValidationError::InvalidUrlUnit);
        }

        if (!is_eof()) {
            std::get<0>(url_.path) += util::percent_encode(*peek(1), PercentEncodeSet::c0_control);
        }
    }
}

// https://url.spec.whatwg.org/#query-state
void UrlParser::state_query() {
    if (auto c = peek(); (!state_override_.has_value() && c == '#') || is_eof()) {
        if (special_schemes.contains(url_.scheme)) {
            url_.query.value() += util::percent_encode(buffer_, PercentEncodeSet::special_query);
        } else {
            url_.query.value() += util::percent_encode(buffer_, PercentEncodeSet::query);
        }

        buffer_.clear();

        if (c == '#') {
            url_.fragment = "";

            state_ = ParserState::Fragment;
        }
    } else if (!is_eof()) {
        if (!is_url_codepoint(util::utf8_to_utf32(remaining_from(0))) && c != '%') {
            validation_error(ValidationError::InvalidUrlUnit);
        }

        if (c == '%'
                && (remaining_from(1).size() < 2 || !util::is_hex_digit(remaining_from(1)[0])
                        || !util::is_hex_digit(remaining_from(1)[1]))) {
            validation_error(ValidationError::InvalidUrlUnit);
        }

        buffer_ += *c;
    }
}

// https://url.spec.whatwg.org/#fragment-state
void UrlParser::state_fragment() {
    if (auto c = peek(); !is_eof()) {
        if (!is_url_codepoint(util::utf8_to_utf32(remaining_from(0))) && c != '%') {
            validation_error(ValidationError::InvalidUrlUnit);
        }

        if (c == '%'
                && (remaining_from(1).size() < 2 || !util::is_hex_digit(remaining_from(1)[0])
                        || !util::is_hex_digit(remaining_from(1)[1]))) {
            validation_error(ValidationError::InvalidUrlUnit);
        }

        url_.fragment.value() += util::percent_encode(*peek(1), PercentEncodeSet::fragment);
    }
}

// https://url.spec.whatwg.org/#concept-domain-to-ascii
std::optional<std::string> UrlParser::domain_to_ascii(std::string_view domain, bool be_strict) const {
    std::string ascii_domain;
    icu::StringByteSink<std::string> tmp{&ascii_domain};

    icu::IDNAInfo inf;
    UErrorCode err = U_ZERO_ERROR;

    std::uint32_t opts = UIDNA_NONTRANSITIONAL_TO_ASCII | UIDNA_CHECK_BIDI | UIDNA_CHECK_CONTEXTJ;

    if (be_strict) {
        opts |= UIDNA_USE_STD3_RULES;
    }

    auto *uts = icu::IDNA::createUTS46Instance(opts, err);

    if (U_FAILURE(err)) {
        spdlog::info("Failed to create UTS46 instance, error {}; idna data probably missing from icu",
                static_cast<std::int64_t>(err));

        return std::nullopt;
    }

    err = U_ZERO_ERROR;

    uts->nameToASCII_UTF8(domain, tmp, inf, err);

    delete uts;

    std::uint32_t proc_err = inf.getErrors();

    // icu doesn't offer a flag to disable VerifyDnsLength or CheckHyphens, so just ignore those failures
    proc_err &= ~UIDNA_ERROR_LEADING_HYPHEN;
    proc_err &= ~UIDNA_ERROR_TRAILING_HYPHEN;
    proc_err &= ~UIDNA_ERROR_HYPHEN_3_4;

    if (!be_strict) {
        proc_err &= ~UIDNA_ERROR_EMPTY_LABEL;
        proc_err &= ~UIDNA_ERROR_LABEL_TOO_LONG;
        proc_err &= ~UIDNA_ERROR_DOMAIN_NAME_TOO_LONG;
    }

    // If domain or any label is empty, proc_err should contain UIDNA_ERROR_EMPTY_LABEL
    if (U_FAILURE(err) || proc_err != 0 || ascii_domain.empty()) {
        validation_error(ValidationError::DomainToAscii);

        return std::nullopt;
    }

    return ascii_domain;
}

// https://url.spec.whatwg.org/#start-with-a-windows-drive-letter
bool UrlParser::starts_with_windows_drive_letter(std::string_view input) const {
    if (input.size() < 2) {
        return false;
    }

    if (!util::is_alpha(input[0]) || (input[1] != ':' && input[1] != '|')) {
        return false;
    }

    if (input.size() == 2) {
        return true;
    }

    if (input.size() > 2 && (input[2] == '/' || input[2] == '\\' || input[2] == '?' || input[2] == '#')) {
        return true;
    }

    return false;
}

// https://url.spec.whatwg.org/#shorten-a-urls-path
void UrlParser::shorten_url_path(Url &url) const {
    assert(!std::holds_alternative<std::string>(url.path));

    if (url.scheme == "file" && std::get<1>(url.path).size() == 1
            && is_normal_windows_drive_letter(std::get<1>(url.path)[0])) {
        return;
    }

    if (!std::get<1>(url.path).empty()) {
        std::get<1>(url.path).pop_back();
    }
}

// https://url.spec.whatwg.org/#concept-host-parser
std::optional<Host> UrlParser::parse_host(std::string_view input, bool is_not_special) const {
    if (input.starts_with("[")) {
        if (!input.ends_with("]")) {
            validation_error(ValidationError::IPv6Unclosed);

            return std::nullopt;
        }

        input.remove_prefix(1);
        input.remove_suffix(1);

        std::optional<std::array<std::uint16_t, 8>> addr = parse_ipv6(input);

        if (!addr.has_value()) {
            return std::nullopt;
        }

        return Host{HostType::Ip6Addr, *addr};
    }

    if (is_not_special) {
        std::optional<std::string> host = parse_opaque_host(input);

        if (!host.has_value()) {
            return std::nullopt;
        }

        return Host{HostType::Opaque, *host};
    }

    assert(!input.empty());

    std::string domain = util::percent_decode(input);

    std::optional<std::string> ascii_domain = domain_to_ascii(domain, false);

    if (!ascii_domain.has_value()) {
        return std::nullopt;
    }

    std::string forbidden = "\t\n\r #/:<>?@[\\]^|";

    for (std::size_t i = 0; i < ascii_domain->size(); i++) {
        if (forbidden.find_first_of(ascii_domain.value()[i]) != std::string::npos || ascii_domain.value()[i] <= 0x1f
                || ascii_domain.value()[i] == '%' || ascii_domain.value()[i] == 0x7f) {
            validation_error(ValidationError::DomainInvalidCodePoint);

            return std::nullopt;
        }
    }

    if (ends_in_number(*ascii_domain)) {
        std::optional<std::uint32_t> ip = parse_ipv4(*ascii_domain);

        if (!ip.has_value()) {
            return std::nullopt;
        }

        return Host{HostType::Ip4Addr, *ip};
    }

    return Host{HostType::DnsDomain, *ascii_domain};
}

// https://url.spec.whatwg.org/#ends-in-a-number-checker
bool UrlParser::ends_in_number(std::string_view input) const {
    // Let parts be the result of strictly splitting input on U+002E (.)
    std::vector<std::string_view> parts = util::split(input, ".");

    if (parts.back().empty()) {
        if (parts.size() == 1) {
            return false;
        }

        parts.pop_back();
    }

    // If last part is non-empty and contains only ASCII digits, return true
    if (!parts.back().empty()) {
        if (std::ranges::all_of(parts.back(), util::is_digit)) {
            return true;
        }
    }

    // If parsing last part as an IPv4 number does not return failure, then return true
    if (parse_ipv4_number(parts.back()).has_value()) {
        return true;
    }

    return false;
}

// https://url.spec.whatwg.org/#concept-ipv4-parser
std::optional<std::uint32_t> UrlParser::parse_ipv4(std::string_view input) const {
    std::vector<std::string_view> parts = util::split(input, ".");

    if (parts.back().empty()) {
        validation_error(ValidationError::IPv4EmptyPart);

        if (parts.size() > 1) {
            parts.pop_back();
        }
    }

    if (parts.size() > 4) {
        validation_error(ValidationError::IPv4TooManyParts);

        return std::nullopt;
    }

    std::vector<std::uint64_t> numbers;

    for (auto part : parts) {
        std::optional<std::tuple<std::uint64_t, bool>> result = parse_ipv4_number(part);

        if (!result.has_value()) {
            validation_error(ValidationError::IPv4NonNumericPart);

            return std::nullopt;
        }

        if (std::get<1>(*result)) {
            validation_error(ValidationError::IPv4NonDecimalPart);
        }

        numbers.emplace_back(std::get<0>(*result));
    }

    for (std::size_t i = 0; i < numbers.size(); i++) {
        if (numbers[i] > 255) {
            validation_error(ValidationError::IPv4OutOfRangePart);

            if (i != numbers.size() - 1) {
                return std::nullopt;
            }
        }
    }

    if (numbers.back() >= std::pow(256, 5 - numbers.size())) {
        return std::nullopt;
    }

    auto ipv4 = static_cast<std::uint32_t>(numbers.back());

    numbers.pop_back();

    for (std::size_t i = 0; i < numbers.size(); i++) {
        ipv4 += static_cast<std::uint32_t>(numbers[i] * std::pow(256, 3 - i));
    }

    return ipv4;
}

// https://url.spec.whatwg.org/#ipv4-number-parser
std::optional<std::tuple<std::uint64_t, bool>> UrlParser::parse_ipv4_number(std::string_view input) const {
    if (input.empty()) {
        return std::nullopt;
    }

    bool v_err = false;
    int r = 10;

    if (input.size() >= 2 && (input.starts_with("0X") || input.starts_with("0x"))) {
        v_err = true;

        input.remove_prefix(2);

        r = 16;
    } else if (input.size() >= 2 && input.starts_with("0")) {
        v_err = true;

        input.remove_prefix(1);

        r = 8;
    }

    if (input.empty()) {
        return {{0, true}};
    }

    for (char i : input) {
        if ((r == 10 && !util::is_digit(i)) || (r == 16 && !util::is_hex_digit(i))
                || (r == 8 && !util::is_octal_digit(i))) {
            return std::nullopt;
        }
    }

    // TODO(zero-one): Differ width based on largest integer value supported by platform?
    std::uint64_t out;

    auto res = std::from_chars(input.data(), input.data() + input.size(), out, r);

    if (res.ec == std::errc::invalid_argument) {
        spdlog::info("Invalid IPv4 number");

        return std::nullopt;
    }

    // This deviation from the spec is necessary, because the spec assumes arbitrary precision
    if (res.ec == std::errc::result_out_of_range) {
        spdlog::info("IPv4 number > 2^64");

        // The number returned here is an error value
        return {{-1, true}};
    }

    return {{out, v_err}};
}

// https://url.spec.whatwg.org/#concept-ipv6-parser
std::optional<std::array<std::uint16_t, 8>> UrlParser::parse_ipv6(std::string_view input) const {
    std::array<std::uint16_t, 8> address = {0, 0, 0, 0, 0, 0, 0, 0};

    std::size_t piece_index = 0;

    std::optional<std::size_t> compress;

    std::size_t pointer = 0;

    if (!input.empty() && input[pointer] == ':') {
        if (!input.substr(1).starts_with(":")) {
            validation_error(ValidationError::IPv6InvalidCompression);

            return std::nullopt;
        }

        pointer += 2;

        piece_index++;

        compress = piece_index;
    }

    while (pointer < input.size()) {
        if (piece_index == 8) {
            validation_error(ValidationError::IPv6TooManyPieces);

            return std::nullopt;
        }

        if (input[pointer] == ':') {
            if (compress.has_value()) {
                validation_error(ValidationError::IPv6MultipleCompression);

                return std::nullopt;
            }

            pointer++;

            piece_index++;

            compress = piece_index;

            continue;
        }

        std::uint64_t value = 0;
        std::size_t length = 0;

        for (; length < 4 && pointer < input.size() && util::is_hex_digit(input[pointer]); pointer++, length++) {
            std::uint64_t out;

            auto res = std::from_chars(input.data() + pointer, input.data() + pointer + 1, out, 16);

            if (res.ec == std::errc::invalid_argument || res.ec == std::errc::result_out_of_range) {
                spdlog::info("Invalid IPv6 input");

                return std::nullopt;
            }

            value = value * 0x10 + out;
        }

        if (pointer < input.size() && input[pointer] == '.') {
            if (length == 0) {
                validation_error(ValidationError::IPv4InIPv6InvalidCodePoint);

                return std::nullopt;
            }

            pointer -= length;

            if (piece_index > 6) {
                validation_error(ValidationError::IPv4InIPv6TooManyPieces);

                return std::nullopt;
            }

            std::size_t numbers_seen = 0;

            while (pointer < input.size()) {
                std::optional<std::uint64_t> ipv4_piece;

                if (numbers_seen > 0) {
                    if (pointer < input.size() && input[pointer] == '.' && numbers_seen < 4) {
                        pointer++;
                    } else {
                        validation_error(ValidationError::IPv4InIPv6InvalidCodePoint);

                        return std::nullopt;
                    }
                }

                if (pointer >= input.size() || !util::is_digit(input[pointer])) {
                    validation_error(ValidationError::IPv4InIPv6InvalidCodePoint);

                    return std::nullopt;
                }

                while (pointer < input.size() && util::is_digit(input[pointer])) {
                    std::uint64_t number;

                    auto res = std::from_chars(input.data() + pointer, input.data() + pointer + 1, number);

                    if (res.ec == std::errc::invalid_argument || res.ec == std::errc::result_out_of_range) {
                        spdlog::info("Invalid IPv6 input 2");

                        return std::nullopt;
                    }

                    if (!ipv4_piece.has_value()) {
                        ipv4_piece = number;
                    } else if (ipv4_piece == 0) {
                        validation_error(ValidationError::IPv4InIPv6InvalidCodePoint);

                        return std::nullopt;
                    } else {
                        ipv4_piece = *ipv4_piece * 10 + number;
                    }

                    if (ipv4_piece > 255) {
                        validation_error(ValidationError::IPv4InIPv6OutOfRangePart);

                        return std::nullopt;
                    }

                    pointer++;
                }

                address[piece_index] = static_cast<std::uint16_t>(address[piece_index] * 0x100ul + *ipv4_piece);

                numbers_seen++;

                if (numbers_seen == 2 || numbers_seen == 4) {
                    piece_index++;
                }
            }

            if (numbers_seen != 4) {
                validation_error(ValidationError::IPv4InIPv6TooFewParts);

                return std::nullopt;
            }

            break;
        } else if (pointer < input.size() && input[pointer] == ':') {
            pointer++;

            if (pointer >= input.size()) {
                validation_error(ValidationError::IPv6InvalidCodePoint);

                return std::nullopt;
            }
        } else if (pointer < input.size()) {
            validation_error(ValidationError::IPv6InvalidCodePoint);

            return std::nullopt;
        }

        address[piece_index] = static_cast<std::uint16_t>(value);

        piece_index++;
    }

    if (compress.has_value()) {
        std::size_t swaps = piece_index - *compress;

        piece_index = 7;

        for (; piece_index != 0 && swaps > 0; piece_index--, swaps--) {
            std::uint16_t tmp = address[piece_index];
            address[piece_index] = address[*compress + swaps - 1];
            address[*compress + swaps - 1] = tmp;
        }
    } else if (!compress.has_value() && piece_index != 8) {
        validation_error(ValidationError::IPv6TooFewPieces);

        return std::nullopt;
    }

    return address;
}

// https://url.spec.whatwg.org/#concept-opaque-host-parser
std::optional<std::string> UrlParser::parse_opaque_host(std::string_view input) const {
    std::string forbidden = "\t\n\r #/:<>?@[\\]^|";

    for (char i : input) {
        if (forbidden.find_first_of(i) != std::string_view::npos || i == '\0') {
            validation_error(ValidationError::HostInvalidCodePoint);

            return std::nullopt;
        }
    }

    std::string_view tmp = input;
    int len = 0;

    while (!tmp.empty()) {
        std::uint32_t cp = util::utf8_to_utf32(tmp);

        len = util::unicode_utf8_byte_count(cp);

        if (!is_url_codepoint(cp)) {
            validation_error(ValidationError::InvalidUrlUnit);
        }

        if (tmp[0] == '%' && tmp.size() > 2 && (!util::is_hex_digit(tmp[1]) || !util::is_hex_digit(tmp[2]))) {
            validation_error(ValidationError::InvalidUrlUnit);
        }

        // I don't *think* this can remove > size(), but maybe i should clamp it anyway
        tmp.remove_prefix(len);
    }

    return util::percent_encode(input, PercentEncodeSet::c0_control);
}

bool UrlParser::is_url_codepoint(std::uint32_t cp) const {
    return cp == '!' || cp == '$' || cp == '&' || cp == '\'' || cp == '(' || cp == ')' || cp == '*' || cp == '+'
            || cp == ',' || cp == '-' || cp == '.' || cp == '/' || cp == ':' || cp == ';' || cp == '=' || cp == '?'
            || cp == '@' || cp == '_' || cp == '~'
            || (cp >= 0x00a0 && cp <= 0x10fffd && !util::is_unicode_noncharacter(cp)
                    && !util::is_unicode_surrogate(cp));
}

// NOLINTEND(bugprone-unchecked-optional-access)
// NOLINTEND(misc-redundant-expression)
} // namespace url
