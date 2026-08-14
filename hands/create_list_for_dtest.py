#!/usr/bin/env python3
"""Create dtest hand-list files.

Generate random deals for use with the dtest program.

Conventionally use ``--seed NNN`` with ``listNNN.txt`` so runs are reproducible.

Duplicate deals within a file, or across files with different seeds,
are possible but vanishingly unlikely.

Example::

  bazel run //hands:create_list_for_dtest -- -n 100 --seed 100 -o hands/list100.txt
"""

from __future__ import annotations

import argparse
import os
import random
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Callable, Iterator, TextIO

from dds3 import (
    analyse_play_pbn,
    calc_all_tables_pbn,
    dealer_par,
    par,
    solve_board_pbn,
)

_RANKS = "AKQJT98765432"
_SUITS = 4
_HANDS = 4
_MAX_DEALS = 100_000
# Each deal runs dozens of DDS solves for PLAY/TRACE; warn above this count.
_LARGE_COUNT_THRESHOLD = 1000

# Intermediate stubs; ``fill_deal_block`` replaces them.
_STUB_TABLE = "TABLE 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 \n"
_STUB_PAR = 'PAR "NS 0" "EW 0" "NS:" "EW:" \n'
_STUB_PAR2 = 'PAR2 "0" "1N-NS" \n'
_STUB_PLAY = 'PLAY 0 "" \n'
_STUB_TRACE = "TRACE 1 0 \n"

_PBN_RE = re.compile(
    r'^PBN\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+"([^"]*)"'
)

_SUIT_CHARS = "SHDC"
_RANK_CHARS = "23456789TJQKA"  # index 0 => rank 2
_SEAT_OFFSET = {"N": 0, "E": 1, "S": 2, "W": 3}


@dataclass(frozen=True)
class DealSpec:
    dealer: int
    vul: int
    trump: int
    first: int
    cards: str  # e.g. N:AKQ.... ...


def _deal_cards(rng: random.Random) -> str:
    """Return a PBN remainCards string starting at North (``N:...``)."""
    deck = [(suit, rank) for suit in range(_SUITS) for rank in range(13)]
    rng.shuffle(deck)
    hands: list[list[list[str]]] = [[[] for _ in range(_SUITS)] for _ in range(_HANDS)]
    for i, (suit, rank_idx) in enumerate(deck):
        hands[i % _HANDS][suit].append(_RANKS[rank_idx])
    parts: list[str] = []
    for hand in hands:
        suit_strs = []
        for suit in range(_SUITS):
            # Keep A-K-Q-... order within each suit.
            ordered = sorted(hand[suit], key=lambda r: _RANKS.index(r))
            suit_strs.append("".join(ordered))
        parts.append(".".join(suit_strs))
    return "N:" + " ".join(parts)


def iter_deals(count: int, *, seed: int) -> Iterator[DealSpec]:
    """Yield ``count`` random deals."""
    if count <= 0:
        raise ValueError("count must be positive")
    rng = random.Random(seed)
    for _ in range(count):
        yield DealSpec(
            dealer=rng.randrange(4),
            vul=rng.randrange(4),
            trump=rng.randrange(5),
            first=rng.randrange(4),
            cards=_deal_cards(rng),
        )


def generate_deals(count: int, *, seed: int) -> list[DealSpec]:
    """Generate ``count`` random deals."""
    return list(iter_deals(count, seed=seed))


def format_fut_line(result: dict[str, Any]) -> str:
    """Serialize a solve result dict to a dtest ``FUT`` line."""
    n = int(result["cards"])
    suits = list(result["suit"])[:n]
    ranks = list(result["rank"])[:n]
    equals = list(result["equals"])[:n]
    scores = list(result["score"])[:n]
    parts = ["FUT", str(n)]
    parts.extend(str(x) for x in suits)
    parts.extend(str(x) for x in ranks)
    parts.extend(str(x) for x in equals)
    parts.extend(str(x) for x in scores)
    return " ".join(parts) + " \n"


def format_pbn_line(deal: DealSpec) -> str:
    return (
        f'PBN {deal.dealer} {deal.vul} {deal.trump} {deal.first} '
        f'"{deal.cards}" \n'
    )


def format_deal_block(deal: DealSpec, fut: dict[str, Any]) -> str:
    return (
        format_pbn_line(deal)
        + format_fut_line(fut)
        + _STUB_TABLE
        + _STUB_PAR
        + _STUB_PAR2
        + _STUB_PLAY
        + _STUB_TRACE
    )


def format_table_line(res_table: list[list[int]]) -> str:
    """Format ``res_table[strain][hand]`` as a dtest ``TABLE`` line."""
    vals: list[str] = []
    for strain in range(5):
        for hand in range(4):
            vals.append(str(res_table[strain][hand]))
    return "TABLE " + " ".join(vals) + " \n"


def format_par_line(par_result: dict) -> str:
    scores = par_result["par_score"]
    contracts = par_result["par_contracts_string"]
    return (
        f'PAR "{scores[0]}" "{scores[1]}" '
        f'"{contracts[0]}" "{contracts[1]}" \n'
    )


def format_par2_line(dealer_par_result: dict) -> str:
    score = dealer_par_result["score"]
    contracts = dealer_par_result["contracts"][: dealer_par_result["number"]]
    quoted = " ".join(f'"{c}"' for c in contracts)
    return f'PAR2 "{score}" {quoted} \n'


def format_play_line(play_cards: str) -> str:
    if len(play_cards) % 2 != 0:
        raise ValueError(f"play string length must be even: {play_cards!r}")
    return f'PLAY {len(play_cards) // 2} "{play_cards}" \n'


def format_trace_line(solved: dict) -> str:
    number = solved["number"]
    if number == 0:
        return "TRACE 0 \n"
    tricks = solved["tricks"][:number]
    body = " ".join(str(t) for t in tricks)
    return f"TRACE {number} {body} \n"


def _parse_pbn_line(line: str) -> tuple[int, int, int, int, str]:
    match = _PBN_RE.match(line.rstrip("\n").rstrip("\r"))
    if match is None:
        raise ValueError(f"unrecognized PBN line: {line!r}")
    return (
        int(match.group(1)),
        int(match.group(2)),
        int(match.group(3)),
        int(match.group(4)),
        match.group(5),
    )


def _rank_to_char(rank: int) -> str:
    if rank < 2 or rank > 14:
        raise ValueError(f"invalid rank {rank}")
    return _RANK_CHARS[rank - 2]


def _char_to_rank(ch: str) -> int:
    idx = _RANK_CHARS.find(ch)
    if idx < 0:
        raise ValueError(f"invalid rank char {ch!r}")
    return idx + 2


def _parse_remain_cards(cards: str) -> list[list[tuple[int, int]]]:
    """Parse a PBN remain-cards string into hands[N,E,S,W] of (suit, rank)."""
    text = cards.strip()
    if len(text) < 2 or text[1] != ":" or text[0] not in _SEAT_OFFSET:
        raise ValueError(f"remain cards must start with N:/E:/S:/W:, got {cards!r}")
    start = _SEAT_OFFSET[text[0]]
    hand_strs = text[2:].split()
    if len(hand_strs) != 4:
        raise ValueError(f"expected 4 hands in remain cards: {cards!r}")

    hands: list[list[tuple[int, int]]] = [[] for _ in range(4)]
    for i, hand_str in enumerate(hand_strs):
        seat = (start + i) % 4
        suits = hand_str.split(".")
        if len(suits) != 4:
            raise ValueError(f"hand must have 4 suits: {hand_str!r}")
        for suit, ranks in enumerate(suits):
            for ch in ranks:
                hands[seat].append((suit, _char_to_rank(ch)))
    return hands


def _hands_to_pbn(hands: list[list[tuple[int, int]]]) -> str:
    parts: list[str] = []
    for seat in range(4):
        by_suit: list[list[int]] = [[] for _ in range(4)]
        for suit, rank in hands[seat]:
            by_suit[suit].append(rank)
        suit_strs = []
        for suit in range(4):
            ordered = sorted(by_suit[suit], reverse=True)
            suit_strs.append("".join(_rank_to_char(r) for r in ordered))
        parts.append(".".join(suit_strs))
    return "N:" + " ".join(parts)


def _trick_winner(
    trick: list[tuple[int, int]],
    trump: int,
    leader: int,
) -> int:
    lead_suit = trick[0][0]
    best = 0
    for i in range(1, 4):
        si, ri = trick[i]
        sb, rb = trick[best]
        i_trump = trump != 4 and si == trump
        b_trump = trump != 4 and sb == trump
        if i_trump and not b_trump:
            best = i
        elif i_trump and b_trump:
            if ri > rb:
                best = i
        elif not i_trump and not b_trump:
            if si == lead_suit and (sb != lead_suit or ri > rb):
                best = i
    return (leader + best) % 4


def generate_dd_play(remain_cards: str, trump: int, first: int) -> str:
    """Return a 52-card DD-optimal play string (suit+rank pairs)."""
    hands = _parse_remain_cards(remain_cards)
    play: list[str] = []
    leader = first
    trick: list[tuple[int, int]] = []

    for _ in range(52):
        player = (leader + len(trick)) % 4
        if not hands[player]:
            raise RuntimeError(f"hand {player} empty before 52 cards")

        cur_suits = [0, 0, 0]
        cur_ranks = [0, 0, 0]
        for i, (suit, rank) in enumerate(trick):
            cur_suits[i] = suit
            cur_ranks[i] = rank

        fut = solve_board_pbn(
            _hands_to_pbn(hands),
            trump=trump,
            first=leader,
            current_trick_suit=tuple(cur_suits),
            current_trick_rank=tuple(cur_ranks),
            target=-1,
            solutions=1,
            mode=1,
        )
        if fut["cards"] < 1:
            raise RuntimeError("solve_board_pbn returned no cards")

        suit = int(fut["suit"][0])
        rank = int(fut["rank"][0])
        card = (suit, rank)
        if card not in hands[player]:
            raise RuntimeError(
                f"solved card { _SUIT_CHARS[suit] }{_rank_to_char(rank) } "
                f"not in hand {player}"
            )

        hands[player].remove(card)
        trick.append(card)
        play.append(_SUIT_CHARS[suit] + _rank_to_char(rank))

        if len(trick) == 4:
            leader = _trick_winner(trick, trump, leader)
            trick.clear()

    return "".join(play)


def _fill_deal_fields(
    dealer: int,
    vul: int,
    trump: int,
    first: int,
    remain: str,
    *,
    calc_fn=None,
) -> dict[str, str]:
    """Compute filled TABLE/PAR/PAR2/PLAY/TRACE lines for one deal."""
    if calc_fn is None:
        calc_fn = calc_all_tables_pbn

    tables = calc_fn([remain])["tables"]
    if len(tables) != 1:
        raise RuntimeError(
            f"calc_all_tables_pbn returned {len(tables)} tables for 1 deal"
        )

    table = tables[0]
    table_dict = {"res_table": table["res_table"]}
    play = generate_dd_play(remain, trump=trump, first=first)
    solved = analyse_play_pbn(remain, play=play, trump=trump, first=first)
    return {
        "TABLE": format_table_line(table["res_table"]),
        "PAR": format_par_line(par(table_dict, vul)),
        "PAR2": format_par2_line(dealer_par(table_dict, dealer, vul)),
        "PLAY": format_play_line(play),
        "TRACE": format_trace_line(solved),
    }


def fill_deal_block(stub_block: str) -> str:
    """Return one deal block with TABLE/PAR/PAR2/PLAY/TRACE filled from DDS."""
    lines = stub_block.splitlines(keepends=True)
    pbn_line = next((line for line in lines if line.startswith("PBN ")), None)
    if pbn_line is None:
        raise ValueError("deal block missing PBN line")

    dealer, vul, trump, first, remain = _parse_pbn_line(pbn_line)
    filled = _fill_deal_fields(dealer, vul, trump, first, remain)

    out: list[str] = []
    for line in lines:
        key = None
        if line.startswith("TABLE "):
            key = "TABLE"
        elif line.startswith("PAR "):
            key = "PAR"
        elif line.startswith("PAR2 "):
            key = "PAR2"
        elif line.startswith("PLAY "):
            key = "PLAY"
        elif line.startswith("TRACE "):
            key = "TRACE"

        if key is None:
            out.append(line)
            continue

        out.append(filled[key])

    return "".join(out)


def format_filled_deal_block(deal: DealSpec, fut: dict[str, Any]) -> str:
    """Build one deal block with TABLE/PAR/PAR2/PLAY/TRACE filled from DDS."""
    return fill_deal_block(format_deal_block(deal, fut))


def solve_fut(
    deal: DealSpec,
    *,
    solve_fn: Callable[..., dict[str, Any]] | None = None,
) -> dict[str, Any]:
    """Solve FUT for one deal matching ``dtest -s solve`` parameters."""
    if solve_fn is None:
        solve_fn = solve_board_pbn

    return solve_fn(
        deal.cards,
        trump=deal.trump,
        first=deal.first,
        target=-1,
        solutions=3,
        mode=1,
    )


def _resolve_output_path(path: Path) -> Path:
    if path.is_absolute():
        return path
    working = os.environ.get("BUILD_WORKING_DIRECTORY")
    if working:
        return Path(working) / path
    return path


def _open_output_stream(output: Path | None) -> tuple[TextIO, bool]:
    """Return ``(stream, should_close)`` for incremental hand-list output."""
    if output is None:
        return (sys.stdout, False)

    path = _resolve_output_path(output)
    path.parent.mkdir(parents=True, exist_ok=True)
    return (path.open("w", encoding="utf-8"), True)


def _large_count_warning(count: int) -> str | None:
    """Return a stderr warning when ``count`` is large enough to be slow."""
    if count < _LARGE_COUNT_THRESHOLD:
        return None
    return (
        f"Warning: generating {count} deals may take a long time "
        f"(each deal runs many DDS solves.)\n"
        f"A fast machine can produce ~5 deals per second."
    )


def _build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument(
        "-n",
        "--count",
        type=int,
        default=10,
        help=(
            "Number of deals (default: 10). Large counts are slow because each "
            "deal runs many DDS solves"
        ),
    )
    p.add_argument(
        "--seed",
        type=int,
        default=1,
        help="RNG seed for reproducibility (default: 1)",
    )
    p.add_argument(
        "-o",
        "--output",
        type=Path,
        default=None,
        help="Output hand-list path (default: stdout)",
    )
    return p


def _parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = _build_parser()
    args = parser.parse_args(argv)
    if args.count <= 0:
        parser.error("count must be positive")
    if args.count > _MAX_DEALS:
        parser.error(f"count must not exceed {_MAX_DEALS}")
    return args


def main(argv: list[str] | None = None) -> int:
    args_list = sys.argv[1:] if argv is None else list(argv)
    if not args_list:
        _build_parser().print_help()
        return 2
    args = _parse_args(args_list)
    warning = _large_count_warning(args.count)
    if warning:
        print(warning, file=sys.stderr)
    stream, should_close = _open_output_stream(args.output)
    try:
        stream.write(f"NUMBER {args.count} \n")
        for i, deal in enumerate(
            iter_deals(args.count, seed=args.seed), start=1
        ):
            if i == 1:
                print("Generating and solving deals…", file=sys.stderr)
            fut = solve_fut(deal)
            stream.write(format_filled_deal_block(deal, fut))
            if i % 100 == 0:
                print(f"  {i}/{args.count} deals…", file=sys.stderr)
    finally:
        if should_close:
            stream.close()

    if args.output is None:
        print(f"Wrote {args.count} deals -> stdout", file=sys.stderr)
    else:
        out = _resolve_output_path(args.output)
        print(f"Wrote {args.count} deals -> {out}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
