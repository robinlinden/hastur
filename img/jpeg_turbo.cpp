// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "img/jpeg_turbo.h"

#include <jpeglib.h>

#include <istream>
#include <optional>

namespace img {

std::optional<JpegTurbo> JpegTurbo::decode(std::istream &) {
    jpeg_decompress_struct d_info{};
    jpeg_error_mgr err_mgr{};

    d_info.err = jpeg_std_error(&err_mgr);
    jpeg_create_decompress(&d_info);

    // TOOD(robinlinden): Decode things.

    jpeg_destroy_decompress(&d_info);
    return {};
}

} // namespace img
