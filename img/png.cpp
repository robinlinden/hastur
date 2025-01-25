// SPDX-FileCopyrightText: 2022-2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "img/png.h"

#include <array>
#include <cassert>
#include <csetjmp>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <optional>
#include <utility>
#include <vector>

#include <png.h>
#include <pngconf.h>

namespace img {
namespace {

constexpr int kSignatureSize = 8;

// Nothing in here can rely on destructors being called while we're using the
// setjmp/longjmp error handling.
void read_png_bytes(png_structp png, png_bytep data, png_size_t length) {
    auto *is = reinterpret_cast<std::istream *>(png_get_io_ptr(png));
    if (!is->read(reinterpret_cast<char *>(data), length)) {
        png_error(png, "failure while reading png data");
    }
}

} // namespace

std::optional<Png> Png::from(std::istream &is) {
    std::array<char, kSignatureSize> signature{};
    is.read(signature.data(), signature.size());
    if (!is || png_sig_cmp(reinterpret_cast<png_const_bytep>(signature.data()), 0, signature.size()) != 0) {
        return std::nullopt;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (png == nullptr) {
        return std::nullopt;
    }

    png_infop info = png_create_info_struct(png);
    if (info == nullptr) {
        png_destroy_read_struct(&png, nullptr, nullptr);
        return std::nullopt;
    }

#ifdef _MSC_VER
    // C4611: interaction between '_setjmp' and C++ object destruction is non-portable.
    // See: https://learn.microsoft.com/en-us/cpp/cpp/using-setjmp-longjmp?view=msvc-170
#pragma warning(disable : 4611)
#endif
    // NOLINTNEXTLINE(cert-err52-cpp): libpng offers us this or aborting.
    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, nullptr);
        return std::nullopt;
    }

    png_set_read_fn(png, reinterpret_cast<void *>(&is), read_png_bytes);
    png_set_sig_bytes(png, kSignatureSize);

    png_read_info(png, info);

    png_set_expand(png);
    png_set_gray_to_rgb(png);
    png_set_add_alpha(png, 0xff, PNG_FILLER_AFTER);
    int const interlacing_passes = png_set_interlace_handling(png);

    png_read_update_info(png, info);

    auto height = png_get_image_height(png, info);
    auto width = png_get_image_width(png, info);
    auto bytes_per_row = png_get_rowbytes(png, info);
    std::vector<unsigned char> bytes;
    assert(bytes_per_row == std::size_t{width} * 4);
    bytes.resize(bytes_per_row * height);

    for (int i = 0; i < interlacing_passes; ++i) {
        for (std::uint32_t row = 0; row < height; ++row) {
            png_read_row(png, bytes.data() + row * bytes_per_row, nullptr);
        }
    }

    Png ret{
            .width = width,
            .height = height,
            .bytes = std::move(bytes),
    };

    png_destroy_read_struct(&png, &info, nullptr);

    return ret;
}

} // namespace img
