#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
#
# SPDX-License-Identifier: BSD-2-Clause

import dataclasses
import operator
import sys
import textwrap
import typing


@dataclasses.dataclass
class Decomposition:
    code_point: int
    decomposes_to: typing.List[int]

    @staticmethod
    def parse_from(line: str) -> "Decomposition":
        code_point, decompositions = operator.itemgetter(0, 5)(line.split(";"))
        return Decomposition(
            int(code_point, base=16),
            [int(n, base=16) for n in decompositions.split(" ")],
        )

    def to_cxx_class(self) -> str:
        decomposition = "".join(f"\\U{c:08X}" for c in self.decomposes_to)
        return f'{{{str(self.code_point)}, "{decomposition}"}}'


# https://www.unicode.org/reports/tr44/#UnicodeData.txt
if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(
            f"Usage: {sys.argv[0]} <UnicodeData.txt>",
            file=sys.stderr,
        )
        sys.exit(1)

    with open(sys.argv[1]) as table:
        lines = table.readlines()

    # Filter out lines not containing decomposition info.
    lines = [line for line in lines if line.split(";")[5].strip()]

    # Filter out non-canonical decompositions.
    lines = [line for line in lines if not line.split(";")[5].startswith("<")]

    decompositions = [Decomposition.parse_from(line) for line in lines]

    sys.stdout.buffer.write(
        textwrap.dedent(
            f"""\
            // SPDX-FileCopyrightText: 2024 Robin Lindén <dev@robinlinden.eu>
            //
            // SPDX-License-Identifier: BSD-2-Clause

            // This file is generated. Do not touch it.

            #ifndef UNICODE_UNICODE_DATA_H_
            #define UNICODE_UNICODE_DATA_H_
            // clang-format off

            #include <array>
            #include <string_view>

            namespace unicode::generated {{

            struct Decomposition {{
                char32_t code_point{{}};
                std::string_view decomposes_to{{}};
                constexpr bool operator==(Decomposition const &) const = default;
            }};

            constexpr std::array<Decomposition, {len(decompositions)}> kDecompositions{{{{
                {",\n                ".join(d.to_cxx_class() for d in decompositions)}
            }}}};

            }} // namespace unicode::generated

            // clang-format on
            #endif
            """
        ).encode()
    )
