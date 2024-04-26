#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
#
# SPDX-License-Identifier: BSD-2-Clause

import dataclasses
import sys
import textwrap
import typing


@dataclasses.dataclass
class Disallowed:
    pass


@dataclasses.dataclass
class DisallowedStd3Valid:
    pass


@dataclasses.dataclass
class DisallowedStd3Mapped:
    maps_to: typing.List[int]

    @staticmethod
    def from_string(s: str) -> "DisallowedStd3Mapped":
        return DisallowedStd3Mapped([int(n, base=16) for n in s.split(" ")])


@dataclasses.dataclass
class Ignored:
    pass


@dataclasses.dataclass
class Mapped:
    maps_to: typing.List[int]

    @staticmethod
    def from_string(s: str) -> "Mapped":
        return Mapped([int(n, base=16) for n in s.split(" ")])


@dataclasses.dataclass
class Deviation:
    maps_to: typing.List[int]

    @staticmethod
    def from_string(s: str) -> "Deviation":
        return Deviation([int(n, base=16) for n in s.split(" ") if len(n) > 0])


@dataclasses.dataclass
class Valid:
    pass


@dataclasses.dataclass
class ValidNv8:
    pass


@dataclasses.dataclass
class ValidXv8:
    pass


Mapping = (
    Disallowed
    | DisallowedStd3Valid
    | DisallowedStd3Mapped
    | Ignored
    | Mapped
    | Deviation
    | Valid
    | ValidNv8
    | ValidXv8
)


class IDNA:
    def __init__(self) -> None:
        # List of each code point starting a new mapping. I.e. if code point 1
        # and 2 are disallowed, and 3 is valid, this list will be
        # `[(1, Disallowed), (3, Valid)]`.
        self.mappings: typing.List[tuple[int, Mapping]] = []

    # https://www.unicode.org/reports/tr46/#Table_Data_File_Fields
    @staticmethod
    def from_table(table_rows: typing.List[str]) -> "IDNA":
        mappings: typing.List[tuple[int, Mapping]] = []
        for row in table_rows:
            # Drop the trailing comment about what code point this is.
            row = row.split("#")[0].strip()

            cols = [col.strip() for col in row.split(";")]
            # Some rows are blank or just a comment.
            if len(cols) <= 1:
                continue

            if ".." in cols[0]:
                code_point = int(cols[0].split("..")[1].lstrip("0"), 16)
            else:
                code_point = int(cols[0].lstrip("0"), 16)

            status = cols[1]
            if status == "disallowed":
                assert len(cols) == 2
                if len(mappings) > 0 and isinstance(mappings[-1][1], Disallowed):
                    mappings[-1] = (code_point, mappings[-1][1])
                    continue
                mappings.append((code_point, Disallowed()))
            elif status == "disallowed_STD3_valid":
                assert len(cols) == 2
                if len(mappings) > 0 and isinstance(
                    mappings[-1][1], DisallowedStd3Valid
                ):
                    mappings[-1] = (code_point, mappings[-1][1])
                    continue
                mappings.append((code_point, DisallowedStd3Valid()))
            elif status == "disallowed_STD3_mapped":
                assert len(cols) == 3
                if len(mappings) > 0 and mappings[-1][
                    1
                ] == DisallowedStd3Mapped.from_string(cols[2]):
                    mappings[-1] = (code_point, mappings[-1][1])
                    continue
                mappings.append((code_point, DisallowedStd3Mapped.from_string(cols[2])))
            elif status == "ignored":
                assert len(cols) == 2
                if len(mappings) > 0 and isinstance(mappings[-1][1], Ignored):
                    mappings[-1] = (code_point, mappings[-1][1])
                    continue
                mappings.append((code_point, Ignored()))
            elif status == "mapped":
                assert len(cols) == 3
                if len(mappings) > 0 and mappings[-1][1] == Mapped.from_string(cols[2]):
                    mappings[-1] = (code_point, mappings[-1][1])
                    continue
                mappings.append((code_point, Mapped.from_string(cols[2])))
            elif status == "deviation":
                assert len(cols) == 3
                if len(mappings) > 0 and mappings[-1][1] == Deviation.from_string(
                    cols[2]
                ):
                    mappings[-1] = (code_point, mappings[-1][1])
                    continue
                mappings.append((code_point, Deviation.from_string(cols[2])))
            elif status == "valid" and len(cols) == 2:
                if len(mappings) > 0 and isinstance(mappings[-1][1], Valid):
                    mappings[-1] = (code_point, mappings[-1][1])
                    continue
                mappings.append((code_point, Valid()))
            elif status == "valid" and len(cols) == 4 and cols[3] == "NV8":
                if len(mappings) > 0 and isinstance(mappings[-1][1], ValidNv8):
                    mappings[-1] = (code_point, mappings[-1][1])
                    continue
                mappings.append((code_point, ValidNv8()))
            elif status == "valid" and len(cols) == 4 and cols[3] == "XV8":
                if len(mappings) > 0 and isinstance(mappings[-1][1], ValidXv8):
                    mappings[-1] = (code_point, mappings[-1][1])
                    continue
                mappings.append((code_point, ValidXv8()))
            else:
                raise Exception(f"Unable to parse data: {cols}")

        idna = IDNA()
        idna.mappings = mappings
        return idna


# The version of MSVC we target doesn't yet support \u{12abc}, so we always
# write the full 8-character escapes.
def to_cxx_variant(a: Mapping) -> str:
    if isinstance(a, Disallowed):
        return "Disallowed{}"
    elif isinstance(a, DisallowedStd3Valid):
        return "DisallowedStd3Valid{}"
    elif isinstance(a, DisallowedStd3Mapped):
        mapping = "".join(f"\\U{c:08X}" for c in a.maps_to)
        return f'DisallowedStd3Mapped{{"{mapping}"}}'
    elif isinstance(a, Ignored):
        return "Ignored{}"
    elif isinstance(a, Mapped):
        mapping = "".join(f"\\U{c:08X}" for c in a.maps_to)
        return f'Mapped{{"{mapping}"}}'
    elif isinstance(a, Deviation):
        mapping = "".join(f"\\U{c:08X}" for c in a.maps_to)
        return f'Deviation{{"{mapping}"}}'
    elif isinstance(a, Valid):
        return "Valid{}"
    elif isinstance(a, ValidNv8):
        return "ValidNv8{}"
    elif isinstance(a, ValidXv8):
        return "ValidXv8{}"
    else:
        raise Exception(f"Unknown mapping: {a}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(
            f"Usage: {sys.argv[0]} <IdnaMappingTable.txt>",
            file=sys.stderr,
        )
        sys.exit(1)

    with open(sys.argv[1]) as table:
        idna = IDNA.from_table(table.readlines())

    sys.stdout.buffer.write(
        textwrap.dedent(
            f"""\
            // SPDX-FileCopyrightText: 2023-2024 Robin Lindén <dev@robinlinden.eu>
            //
            // SPDX-License-Identifier: BSD-2-Clause

            // This file is generated. Do not touch it.

            #ifndef IDNA_IDNA_DATA_H_
            #define IDNA_IDNA_DATA_H_
            // clang-format off

            #include <array>
            #include <string_view>
            #include <variant>
            #include <utility>

            namespace idna::uts46 {{

            struct Disallowed {{
                constexpr bool operator==(Disallowed const &) const = default;
            }};

            struct DisallowedStd3Valid {{
                constexpr bool operator==(DisallowedStd3Valid const &) const = default;
            }};

            struct DisallowedStd3Mapped {{
                std::string_view maps_to;
                constexpr bool operator==(DisallowedStd3Mapped const &) const = default;
            }};

            struct Ignored {{
                constexpr bool operator==(Ignored const &) const = default;
            }};

            struct Mapped {{
                std::string_view maps_to;
                constexpr bool operator==(Mapped const &) const = default;
            }};

            struct Deviation {{
                std::string_view maps_to;
                constexpr bool operator==(Deviation const &) const = default;
            }};

            struct Valid {{
                constexpr bool operator==(Valid const &) const = default;
            }};

            struct ValidNv8 {{
                constexpr bool operator==(ValidNv8 const &) const = default;
            }};

            struct ValidXv8 {{
                constexpr bool operator==(ValidXv8 const &) const = default;
            }};

            using Mapping = std::variant<
                    Disallowed,
                    DisallowedStd3Valid,
                    DisallowedStd3Mapped,
                    Ignored,
                    Mapped,
                    Deviation,
                    Valid,
                    ValidNv8,
                    ValidXv8>;

            constexpr std::array<std::pair<char32_t, Mapping>, {len(idna.mappings)}> kMappings{{{{
                {",\n                ".join("{" + str(c[0]) + ", Mapping{" + to_cxx_variant(c[1]) + "}}" for c in idna.mappings)}
            }}}};

            }} // namespace idna::uts46

            // clang-format on
            #endif
            """
        ).encode()
    )
