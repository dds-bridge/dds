#!/usr/bin/env python3
"""Print the double-dummy table and par for a deal from PBN on the CLI or a file.

Python counterpart to examples/dd_table_for_deal.cpp.
"""

from __future__ import annotations

import os
import re
import sys
from pathlib import Path

from dds3 import calc_all_tables_pbn, calc_par_from_table

PBN_FILE_MAX = 16 * 1024 * 1024  # safety cap for whole PBN exports
PBN_DEAL_MAX = 80  # matches DdTableDealPBN::cards

_CONTRACT_RE = re.compile(
    r"^(N|E|S|W|NS|EW)\s+(\d+)([SHDCN])(x)?$",
    re.IGNORECASE,
)

_VULNERABLE_ALIASES = {
    "none": 0,
    "0": 0,
    "both": 1,
    "1": 1,
    "ns": 2,
    "2": 2,
    "ew": 3,
    "3": 3,
}

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
    data = stream.read(PBN_FILE_MAX + 1)
    if not data:
        return None
    text = data if isinstance(data, str) else data.decode("utf-8", errors="replace")
    if len(text) > PBN_FILE_MAX:
        raise ValueError(f"PBN input too large (max {PBN_FILE_MAX} characters)")
    return text


def _read_pbn_file(path: str) -> str | None:
    candidates = [Path(path)]
    workspace = os.environ.get("BUILD_WORKSPACE_DIRECTORY")
    if workspace is not None:
        candidates.append(Path(workspace) / path)
    for candidate in candidates:
        try:
            text = candidate.read_text(encoding="utf-8", errors="replace")
            if len(text) > PBN_FILE_MAX:
                raise ValueError(f"PBN file too large (max {PBN_FILE_MAX} characters)")
            return text
        except OSError:
            continue
    return None


def _extract_deal_tags(text: str) -> list[str]:
    return [match.group(1) for match in _DEAL_TAG_RE.finditer(text)]


def _unique_deals(deals: list[str]) -> list[str]:
    """Return deals in first-seen order with exact duplicates removed."""
    unique: list[str] = []
    seen: set[str] = set()
    for deal in deals:
        if deal in seen:
            continue
        seen.add(deal)
        unique.append(deal)
    return unique


def _looks_like_path(arg: str) -> bool:
    """True when arg is more likely a filename than a raw PBN deal string."""
    if "/" in arg or "\\" in arg:
        return True
    lower = arg.lower()
    return lower.endswith(".pbn") or lower.endswith(".txt")


def _load_deals(arg: str) -> list[str]:
    if arg == "-":
        text = _read_pbn_stream(sys.stdin)
        if text is None:
            raise ValueError("No PBN input on stdin")
        source = "stdin"
    else:
        text = _read_pbn_file(arg)
        source = arg if text is not None else None

    if source is not None:
        deals = _extract_deal_tags(text)
        if not deals:
            raise ValueError(f'No [Deal "..."] tag found in {source}')
        return deals

    if _looks_like_path(arg):
        raise ValueError(f"Cannot read file: {arg}")

    if len(arg) >= PBN_DEAL_MAX:
        raise ValueError(f"PBN deal too long (max {PBN_DEAL_MAX - 1} characters)")
    return [arg]


def _parse_vulnerable(text: str) -> int:
    try:
        return _VULNERABLE_ALIASES[text.lower()]
    except KeyError as exc:
        raise ValueError(
            "Invalid --vul value (use none|both|ns|ew or 0|1|2|3)"
        ) from exc


def _parse_limit(text: str) -> int:
    if not text.isdigit() or int(text) < 1:
        raise ValueError("Invalid --limit value (use a positive integer)")
    return int(text)


def _parse_cli(argv: list[str]) -> tuple[str, int, int | None] | None:
    """Return (deal_arg, vulnerable, limit) or None for help.

    limit is None when unrestricted. Raises ValueError on bad args.
    """
    deal: str | None = None
    vulnerable = 0
    limit: int | None = None
    i = 1
    while i < len(argv):
        arg = argv[i]
        if arg in ("-h", "--help"):
            return None
        if arg == "--vul":
            if i + 1 >= len(argv):
                raise ValueError("--vul requires a value (none|both|ns|ew or 0|1|2|3)")
            vulnerable = _parse_vulnerable(argv[i + 1])
            i += 2
            continue
        if arg == "--limit":
            if i + 1 >= len(argv):
                raise ValueError("--limit requires a positive integer")
            limit = _parse_limit(argv[i + 1])
            i += 2
            continue
        if arg.startswith("-") and arg != "-":
            raise ValueError(f"Unknown option: {arg}")
        if deal is not None:
            raise ValueError("Only one deal argument is allowed")
        deal = arg
        i += 1

    if deal is None:
        if not sys.stdin.isatty():
            deal = "-"
        else:
            raise ValueError("missing deal argument")

    return deal, vulnerable, limit


def _print_usage(prog: str) -> None:
    print(
        f"Usage: {prog} [--vul none|both|ns|ew|0|1|2|3] [--limit N] "
        f"<pbn_deal_or_file>\n"
        f"       {prog} -h | --help\n"
        "\n"
        "Calculate double-dummy tricks and par for all strains and leads.\n"
        "\n"
        "Arguments:\n"
        "  <pbn_deal_or_file>  DDS PBN deal string, or path to a .pbn file\n"
        "  --vul              Vulnerability: none|both|ns|ew or 0|1|2|3"
        " (default: none)\n"
        "  --limit            Solve only the first N unique deals\n"
        "\n"
        'If stdin is not a terminal, PBN is read from stdin (all [Deal "..."] tags).\n'
        "\n"
        "Examples:\n"
        f'  {prog} "N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 '
        f'5.A95432.7632.K6 AKJ9842.K.T8.J93"\n'
        f"  {prog} --vul ns hands/example.pbn\n"
        f"  {prog} --limit 3 hands/multi_board.pbn\n"
        f"  {prog} < hands/example.pbn\n",
        file=sys.stderr,
    )


def _rawscore_undertricks(tricks: int, is_vul: bool) -> int:
    """Match library/src/par.cpp rawscore(-1, tricks, is_vul)."""
    if is_vul:
        return -300 * tricks + 100
    if tricks <= 3:
        return -200 * tricks + 100
    return -300 * tricks + 400


def _side_vulnerable(seats: str, vulnerable: int) -> bool:
    if vulnerable == 1:
        return True
    if seats in ("NS", "N", "S"):
        return vulnerable == 2
    if seats in ("EW", "E", "W"):
        return vulnerable == 3
    return False


def _sacrifice_undertricks(score: int, vulnerable: int, seats: str) -> int:
    """Infer sacrifice undertricks from the declaring-side par score."""
    is_vul = _side_vulnerable(seats, vulnerable)
    for tricks in range(1, 14):
        sacrifice_score = _rawscore_undertricks(tricks, is_vul)
        if sacrifice_score == score or -sacrifice_score == score:
            return tricks
    raise ValueError(f"cannot infer undertricks for par score {score}")


def _contract_body(side_contracts: str) -> str | None:
    if ":" not in side_contracts:
        return None
    body = side_contracts.split(":", 1)[1].strip()
    return body or None


def _normalize_contract_piece(piece: str) -> tuple[str, str, str] | None:
    """Expand DDS multi-level encodings like 'EW 45S' into ('EW', '4S', '+1')."""
    match = _CONTRACT_RE.match(piece.strip())
    if match is None:
        return None

    seats, levels, denom, doubled = match.groups()
    seats = seats.upper()
    denom = denom.upper()
    digits = [int(ch) for ch in levels]
    level = digits[0]
    over = digits[-1] - digits[0]

    contract = f"{level}{denom}{'x' if doubled else ''}"
    if doubled:
        return seats, contract, "="

    result = f"+{over}" if over > 0 else "="
    return seats, contract, result


def _normalize_par_body(body: str) -> tuple[str, str] | None:
    """Normalize comma-separated contracts; result comes from the first make."""
    pieces = [p.strip() for p in body.split(",") if p.strip()]
    if not pieces:
        return None

    normalized: list[str] = []
    result = "="
    for i, piece in enumerate(pieces):
        parsed = _normalize_contract_piece(piece)
        if parsed is None:
            return None
        seats, contract, piece_result = parsed
        if i == 0:
            normalized.append(f"{seats} {contract}")
            result = piece_result
        else:
            normalized.append(contract)
    return ", ".join(normalized), result


def _first_seats(body: str) -> str | None:
    match = _CONTRACT_RE.match(body.split(",", 1)[0].strip())
    if match is None:
        return None
    return match.group(1).upper()


def _declaring_score(par_results: dict, seats: str) -> int:
    scores = par_results["par_score"]
    ns_score = int(scores[0].split()[1])
    ew_score = int(scores[1].split()[1])
    if seats in ("NS", "N", "S"):
        return ns_score
    return ew_score


def _format_par_line(par_results: dict, *, vulnerable: int) -> str | None:
    """Return a one-line par summary for the declaring side."""
    scores = par_results["par_score"]
    contracts = par_results["par_contracts_string"]

    ns_score = int(scores[0].split()[1])
    ew_score = int(scores[1].split()[1])
    if ns_score == 0 and ew_score == 0:
        return "Par: 0"

    body = _contract_body(contracts[0])
    if body is None:
        return None

    seats = _first_seats(body)
    if seats is None:
        return None

    normalized = _normalize_par_body(body)
    if normalized is None:
        return None
    body, make_result = normalized

    score = _declaring_score(par_results, seats)
    is_sacrifice = "x" in body.lower()
    if is_sacrifice:
        result = f"-{_sacrifice_undertricks(score, vulnerable, seats)}"
    else:
        result = make_result

    return f"Par: {body} {result} {score}"


def _print_par_verbose(par_results: dict) -> None:
    """Match examples/hands.cpp print_par."""
    scores = par_results["par_score"]
    contracts = par_results["par_contracts_string"]
    print(f"NS score: {scores[0]}")
    print(f"EW score: {scores[1]}")
    print(f"NS list : {contracts[0]}")
    print(f"EW list : {contracts[1]}")
    print()


def _print_par(par_results: dict, *, vulnerable: int = 0) -> None:
    line = _format_par_line(par_results, vulnerable=vulnerable)
    if line is None:
        _print_par_verbose(par_results)
    else:
        print(line)


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
        and bp < len(pbn_deal)
        and pbn_deal[bp] not in "NWESnwes"
    ):
        bp += 1
    if bp >= 3 or bp >= len(pbn_deal):
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
    print()


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

    try:
        parsed = _parse_cli(argv)
    except ValueError as exc:
        print(exc, file=sys.stderr)
        _print_usage(prog)
        return 1

    if parsed is None:
        _print_usage(prog)
        return 0

    input_arg, vulnerable, limit = parsed

    try:
        pbn_deals = _unique_deals(_load_deals(input_arg))
    except ValueError as exc:
        print(exc, file=sys.stderr)
        return 1

    if limit is not None:
        pbn_deals = pbn_deals[:limit]

    if any(len(deal) >= PBN_DEAL_MAX for deal in pbn_deals):
        print(
            f"PBN deal too long (max {PBN_DEAL_MAX - 1} characters)",
            file=sys.stderr,
        )
        return 1

    deal_count = len(pbn_deals)
    for deal_no, pbn_deal in enumerate(pbn_deals, start=1):
        try:
            result = calc_all_tables_pbn([pbn_deal])
        except (ValueError, RuntimeError) as exc:
            print(f"DDS error: {exc}", file=sys.stderr)
            return 1

        tables = result.get("tables")
        if not tables:
            print("DDS error: no table returned", file=sys.stderr)
            return 1

        table = tables[0]
        try:
            par_results = calc_par_from_table(table, vulnerable=vulnerable)
        except (ValueError, RuntimeError) as exc:
            print(f"DDS error: {exc}", file=sys.stderr)
            return 1

        title = (
            "dd_table_for_deal:\n"
            if deal_count == 1
            else f"Deal {deal_no}:\n"
        )
        _print_pbn_hand(title, pbn_deal)
        _print_table(table["res_table"])
        _print_par(par_results, vulnerable=vulnerable)
        if deal_count > 1:
            print()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
