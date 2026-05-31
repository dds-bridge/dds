#!/usr/bin/env python3
"""Print the double-dummy table for a deal from PBN on the command line or a file.

Python counterpart to examples/dd_table_for_deal.cpp.
"""

from __future__ import annotations

import os
import re
import sys
from pathlib import Path

from dds3 import calc_all_tables_pbn

PBN_FILE_MAX = 8192

# res_table[strain][hand]: strain 0-3 = S,H,D,C; 4 = NT. Columns match C++ print_table.
_STRAIN_ROWS = (("NT", 4), ("S", 0), ("H", 1), ("D", 2), ("C", 3))
_HAND_COLUMNS = (("North", 0), ("South", 2), ("East", 1), ("West", 3))

_DDS_FULL_LINE = 80
_DDS_HAND_OFFSET = 12
_DDS_HAND_LINES = 12

_BIT_MAP_RANK = [
    0x0000, 0x0000, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020,
    0x0040, 0x0080, 0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000,
]
_CARD_RANK_CHARS = "xx23456789TJQKA-"

_DEAL_TAG_RE = re.compile(r'\[Deal\s*"([^"]*)"', re.IGNORECASE)


def _read_pbn_stream(stream) -> str | None:
    data = stream.read(PBN_FILE_MAX - 1)
    if not data:
        return None
    return data if isinstance(data, str) else data.decode("utf-8", errors="replace")


def _read_pbn_file(path: str) -> str | None:
    candidates = [Path(path)]
    workspace = os.environ.get("BUILD_WORKSPACE_DIRECTORY")
    if workspace is not None:
        candidates.append(Path(workspace) / path)
    for candidate in candidates:
        try:
            return candidate.read_text(encoding="utf-8", errors="replace")[
                : PBN_FILE_MAX - 1
            ]
        except OSError:
            continue
    return None


def _extract_deal_tag(text: str) -> str | None:
    match = _DEAL_TAG_RE.search(text)
    return match.group(1) if match else None


def _load_deal(arg: str) -> str:
    if arg == "-":
        text = _read_pbn_stream(sys.stdin)
        if text is None:
            raise ValueError("No PBN input on stdin")
        source = "stdin"
    else:
        text = _read_pbn_file(arg)
        source = arg if text is not None else None

    if source is not None:
        deal = _extract_deal_tag(text)
        if deal is None:
            raise ValueError(f'No [Deal "..."] tag found in {source}')
        return deal

    if len(arg) >= PBN_FILE_MAX:
        raise ValueError(f"PBN deal too long (max {PBN_FILE_MAX - 1} characters)")
    return arg


def _print_usage(prog: str) -> None:
    print(
        f"Usage: {prog} <pbn_deal_or_file>\n"
        f"       {prog} -h | --help\n"
        "\n"
        "Calculate double-dummy tricks for all strains and leads.\n"
        "\n"
        "Arguments:\n"
        "  <pbn_deal_or_file>  DDS PBN deal string, or path to a .pbn file\n"
        "\n"
        'If stdin is not a terminal, PBN is read from stdin (uses [Deal "..."]).\n'
        "\n"
        "Examples:\n"
        f'  {prog} "N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 '
        f'5.A95432.7632.K6 AKJ9842.K.T8.J93"\n'
        f"  {prog} hands/example.pbn\n"
        f"  {prog} < hands/example.pbn\n",
        file=sys.stderr,
    )


def _is_card(ch: str) -> int:
    ch = ch.upper()
    ranks = "23456789TJQKA"
    return ranks.index(ch) + 2 if ch in ranks else 0


def _convert_pbn(pbn_deal: str) -> list[list[int]]:
    """Match examples/hands.cpp convert_pbn (4 hands x 4 suits bitmasks)."""
    remain = [[0] * 4 for _ in range(4)]
    bp = 0
    while (
        bp < 3
        and pbn_deal[bp] not in "NWESnwes"
    ):
        bp += 1
    if bp >= 3:
        return remain

    first = {"N": 0, "E": 1, "S": 2, "W": 3}[pbn_deal[bp].upper()]
    bp += 2
    hand_rel_first = 0
    suit_in_hand = 0

    while bp < 80 and bp < len(pbn_deal):
        ch = pbn_deal[bp]
        card = _is_card(ch)
        if card:
            if first == 0:
                hand = hand_rel_first
            elif first == 1:
                hand = 1 if hand_rel_first == 0 else 0 if hand_rel_first == 3 else hand_rel_first + 1
            elif first == 2:
                hand = 2 if hand_rel_first == 0 else 3 if hand_rel_first == 1 else hand_rel_first - 2
            else:
                hand = 3 if hand_rel_first == 0 else hand_rel_first - 1
            remain[hand][suit_in_hand] |= _BIT_MAP_RANK[card] << 2
        elif ch == ".":
            suit_in_hand += 1
        elif ch == " ":
            hand_rel_first += 1
            suit_in_hand = 0
        bp += 1
    return remain


def _print_pbn_hand(title: str, pbn_deal: str) -> None:
    """Match examples/hands.cpp print_pbn_hand / print_hand."""
    remain_cards = _convert_pbn(pbn_deal)
    text = [[" "] * _DDS_FULL_LINE for _ in range(_DDS_HAND_LINES)]
    row_ends = [_DDS_FULL_LINE] * _DDS_HAND_LINES

    for h in range(4):
        if h == 0:
            offset, line = _DDS_HAND_OFFSET, 0
        elif h == 1:
            offset, line = 2 * _DDS_HAND_OFFSET, 4
        elif h == 2:
            offset, line = _DDS_HAND_OFFSET, 8
        else:
            offset, line = 0, 4

        for s in range(4):
            row = line + s
            c = offset
            for r in range(14, 1, -1):
                if (remain_cards[h][s] >> 2) & _BIT_MAP_RANK[r]:
                    text[row][c] = _CARD_RANK_CHARS[r]
                    c += 1
            if c == offset:
                text[row][c] = "-"
                c += 1
            if h != 3:
                row_ends[row] = c

    sys.stdout.write(title)
    dash_len = max(0, len(title) - 1)
    print("-" * dash_len)
    for i in range(_DDS_HAND_LINES):
        print("".join(text[i][: row_ends[i]]))
    print("\n")


def _print_table(res_table: list[list[int]]) -> None:
    """Match examples/hands.cpp print_table (%5s %-5s ... / %5c %5d ...)."""
    print(f"{'':>5} {'North':<5} {'South':<5} {'East':<5} {'West':<5}")

    _, nt_strain = _STRAIN_ROWS[0]
    print(
        f"{'NT':>5} "
        f"{res_table[nt_strain][_HAND_COLUMNS[0][1]]:5d} "
        f"{res_table[nt_strain][_HAND_COLUMNS[1][1]]:5d} "
        f"{res_table[nt_strain][_HAND_COLUMNS[2][1]]:5d} "
        f"{res_table[nt_strain][_HAND_COLUMNS[3][1]]:5d}"
    )

    for label, strain in _STRAIN_ROWS[1:]:
        print(
            f"{label:>5} "
            f"{res_table[strain][_HAND_COLUMNS[0][1]]:5d} "
            f"{res_table[strain][_HAND_COLUMNS[1][1]]:5d} "
            f"{res_table[strain][_HAND_COLUMNS[2][1]]:5d} "
            f"{res_table[strain][_HAND_COLUMNS[3][1]]:5d}"
        )
    print()


def main(argv: list[str] | None = None) -> int:
    argv = list(sys.argv if argv is None else argv)
    prog = Path(argv[0]).name

    if len(argv) == 2:
        if argv[1] in ("-h", "--help"):
            _print_usage(prog)
            return 0
        input_arg = argv[1]
    elif len(argv) == 1 and not sys.stdin.isatty():
        input_arg = "-"
    else:
        _print_usage(prog)
        return 1

    try:
        pbn_deal = _load_deal(input_arg)
    except ValueError as exc:
        print(exc, file=sys.stderr)
        return 1

    try:
        result = calc_all_tables_pbn([pbn_deal])
    except (ValueError, RuntimeError) as exc:
        print(f"DDS error: {exc}", file=sys.stderr)
        return 1

    tables = result.get("tables")
    if not tables:
        print("DDS error: no table returned", file=sys.stderr)
        return 1

    res_table = tables[0]["res_table"]
    _print_pbn_hand("dd_table_for_deal:\n", pbn_deal)
    _print_table(res_table)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
