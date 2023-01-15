// SPDX-FileCopyrightText: 2023 David Zero <zero-one@zer0-one.net>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef NET_IP_H_
#define NET_IP_H_

#include <cstdint>
#include <span>
#include <string>

namespace net {

std::string ipv4_serialize(std::uint32_t addr);
std::string ipv6_serialize(std::span<std::uint16_t, 8> addr);

} // namespace net

#endif
