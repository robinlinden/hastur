// SPDX-FileCopyrightText: 2022-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef UTIL_ARG_PARSER_H_
#define UTIL_ARG_PARSER_H_

#include <tl/expected.hpp>

#include <charconv>
#include <concepts>
#include <cstdint>
#include <format>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace util {

struct ArgParseError {
    enum class Code : std::uint8_t {
        InvalidArgument,
        MissingArgument,
        UnhandledArgument,
    };

    Code code{};
    std::string message;
};

class ArgParser {
public:
    template<std::integral ArgT>
    [[nodiscard]] ArgParser &argument(std::string_view long_option, ArgT &out) {
        long_[long_option] = [&out](std::string_view argument) {
            auto [ptr, ec] = std::from_chars(argument.data(), argument.data() + argument.size(), out);
            if (ec != std::errc{} || ptr != argument.data() + argument.size()) {
                return Status::Failure;
            }

            return Status::Success;
        };
        return *this;
    }

    [[nodiscard]] ArgParser &argument(std::string_view arg, bool &was_passed) {
        store_true_[arg] = [&was_passed]() {
            was_passed = true;
        };
        return *this;
    }

    [[nodiscard]] ArgParser &argument(std::string_view arg, std::string &out) {
        long_[arg] = [&out](std::string_view argument) {
            out.assign(argument);
            return Status::Success;
        };
        return *this;
    }

    [[nodiscard]] ArgParser &positional(std::string &out) {
        positional_.emplace_back([&out](std::string_view argument) { out = std::string{argument}; });
        return *this;
    }

    [[nodiscard]] tl::expected<void, ArgParseError> parse(int argc, char const *const *argv) {
        for (int i = 1; i < argc; ++i) {
            auto arg = std::string_view{argv[i]};
            if (long_.contains(arg)) {
                if (i + 1 == argc) {
                    return tl::unexpected(ArgParseError{
                            ArgParseError::Code::MissingArgument,
                            std::format("Missing argument for {}", arg),
                    });
                }

                auto argarg = std::string_view{argv[i + 1]};
                if (long_.at(arg)(argarg) != Status::Success) {
                    return tl::unexpected(ArgParseError{
                            ArgParseError::Code::InvalidArgument,
                            std::format("Invalid argument for {}: {}", arg, argarg),
                    });
                }

                ++i;
                continue;
            }

            if (store_true_.contains(arg)) {
                store_true_.at(arg)();
                continue;
            }

            int maybe_positional = i + static_cast<int>(positional_.size()) - argc;
            if (maybe_positional >= 0 && std::cmp_less(maybe_positional, positional_.size())) {
                positional_[maybe_positional](arg);
                continue;
            }

            return tl::unexpected(ArgParseError{
                    ArgParseError::Code::UnhandledArgument,
                    std::format("Unhandled argument: {}", arg),
            });
        }

        return {};
    }

private:
    enum class Status : std::uint8_t {
        Success,
        Failure,
    };

    std::map<std::string_view, std::function<Status(std::string_view)>> long_;
    std::map<std::string_view, std::function<void()>> store_true_;
    std::vector<std::function<void(std::string_view)>> positional_;
};

} // namespace util

#endif
