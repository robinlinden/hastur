// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "img/jpeg_turbo.h"

#include <jpeglib.h>

#include <csetjmp>
#include <cstddef>
#include <optional>
#include <span>
#include <tuple>
#include <utility>
#include <vector>

namespace img {
namespace {

#ifdef _MSC_VER
// C4611: interaction between '_setjmp' and C++ object destruction is non-portable.
// See: https://learn.microsoft.com/en-us/cpp/cpp/using-setjmp-longjmp?view=msvc-170
#pragma warning(disable : 4611)
#endif

struct ErrorHandler {
    std::jmp_buf setjmp_buffer;
    jpeg_error_mgr err_mgr;
};

} // namespace

std::optional<JpegTurbo> JpegTurbo::from(std::span<std::byte const> data) {
    jpeg_decompress_struct d_info{};
    ErrorHandler err{};
    d_info.client_data = &err;

    d_info.err = jpeg_std_error(&err.err_mgr);
    err.err_mgr.error_exit = [](j_common_ptr cinfo) {
        auto *eh = reinterpret_cast<ErrorHandler *>(cinfo->client_data);
        // NOLINTNEXTLINE(cert-err52-cpp): libjpeg-turbo offers us this or aborting.
        std::longjmp(eh->setjmp_buffer, 1);
    };

    // NOLINTNEXTLINE(cert-err52-cpp): libjpeg-turbo offers us this or aborting.
    if (setjmp(err.setjmp_buffer)) {
        jpeg_destroy_decompress(&d_info);
        return {};
    }

    jpeg_create_decompress(&d_info);

    jpeg_mem_src(&d_info,
            reinterpret_cast<unsigned char const *>(data.data()),
            static_cast<unsigned long>(data.size())); // NOLINT(google-runtime-int): Not our API.

    // This will only ever fail in a way that calls error_exit when using a
    // nonsuspending data source and require_image=true.
    std::ignore = jpeg_read_header(&d_info, TRUE);
    d_info.out_color_space = JCS_EXT_RGBA;

    std::vector<unsigned char> bytes{};
    bytes.resize(std::size_t{d_info.image_width} * d_info.image_height * 4);

    // This will never fail as our data source will never suspend.
    std::ignore = jpeg_start_decompress(&d_info);

    while (d_info.output_scanline < d_info.output_height) {
        std::size_t row = d_info.output_scanline;
        auto *row_ptr = bytes.data() + row * d_info.output_width * 4;
        if (jpeg_read_scanlines(&d_info, &row_ptr, 1) != 1) {
            return std::nullopt;
        }
    }

    jpeg_destroy_decompress(&d_info);
    return JpegTurbo{d_info.image_width, d_info.image_height, std::move(bytes)};
}

} // namespace img
