// SPDX-FileCopyrightText: 2023 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "archive/zlib.h"

#include <zlib.h>

#include <utility>

namespace archive {

tl::expected<std::string, ZlibError> zlib_decode(std::string_view data, ZlibMode mode) {
    z_stream s{
            .next_in = reinterpret_cast<Bytef const *>(data.data()),
            .avail_in = static_cast<uInt>(data.size()),
    };

    // https://github.com/madler/zlib/blob/v1.2.13/zlib.h#L832
    // The windowBits parameter is the base two logarithm of the
    // maximum window size (the size of the history buffer). It
    // should be in the range 8..15 for this version of the library.
    // <...>
    // windowBits can also be greater than 15 for optional gzip
    // decoding. Add 32 to windowBits to enable zlib and gzip
    // decoding with automatic header detection, or add 16 to decode
    // only the gzip format <...>.
    int const zlib_mode = [mode] {
        switch (mode) {
            case ZlibMode::Gzip:
                return 15;
            default:
            case ZlibMode::Zlib:
                return 0;
        }
    }();
    constexpr int kWindowBits = 15;
    if (auto error = inflateInit2(&s, kWindowBits + zlib_mode); error != Z_OK) {
        return tl::unexpected{ZlibError{.message = "inflateInit2", .code = error}};
    }

    std::string out{};
    std::string buf{};
    constexpr auto kZlibInflateChunkSize = std::size_t{64} * 1024; // Chosen by a fair dice roll.
    buf.resize(kZlibInflateChunkSize);
    do {
        s.next_out = reinterpret_cast<Bytef *>(buf.data());
        s.avail_out = static_cast<uInt>(buf.size());
        int ret = inflate(&s, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END) {
            std::string msg;
            if (s.msg != nullptr) {
                msg = s.msg;
            }
            inflateEnd(&s);
            return tl::unexpected{ZlibError{.message = std::move(msg), .code = ret}};
        }

        uInt inflated_bytes = static_cast<uInt>(buf.size()) - s.avail_out;
        out += buf.substr(0, inflated_bytes);
    } while (s.avail_out == 0);

    inflateEnd(&s);
    return out;
}

} // namespace archive
