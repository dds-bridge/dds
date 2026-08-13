#!/usr/bin/env python3
"""Create dtest hand-list files.

Generate unique random deals (``-n`` / ``--seed``) with FUT solved for
``dtest -s solve`` and TABLE/PAR/PAR2/PLAY/TRACE filled from DDS.

Examples::

  bazel run //hands:create_list_for_dtest -- -n 10000 --seed 1 -o hands/random10000.txt
  bazel run //hands:create_list_for_dtest -- -n 3 --seed 1 > hands/random3.txt
"""

from __future__ import annotations

import argparse
import os
import random
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Callable, Sequence, TextIO

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
_SEATS = "NESW"
_BATCH = 200  # MAXNOOFBOARDS

# Intermediate stubs; ``build_hand_list`` replaces them via fill_fn.
_STUB_TABLE = "TABLE 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 \n"
_STUB_PAR = 'PAR "NS 0" "EW 0" "NS:" "EW:" \n'
_STUB_PAR2 = 'PAR2 "0" "1N-NS" \n'
_STUB_PLAY = 'PLAY 0 "" \n'
_STUB_TRACE = "TRACE 1 0 \n"

# calc_all_tables_pbn rejects batches larger than MAXNOOFTABLES when all
# strains are included (40 * 5 / 5).
MAX_TABLES_PER_BATCH = 40

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


def generate_unique_deals(count: int, *, seed: int) -> list[DealSpec]:
    """Generate ``count`` deals with unique card layouts."""
    if count <= 0:
        raise ValueError("count must be positive")
    rng = random.Random(seed)
    seen: set[str] = set()
    deals: list[DealSpec] = []
    # Bound retries; collisions are vanishingly rare for 10k of ~52!/ (13!)^4.
    attempts = 0
    max_attempts = count * 20 + 1000
    while len(deals) < count:
        attempts += 1
        if attempts > max_attempts:
            raise RuntimeError(f"failed to generate {count} unique deals")
        cards = _deal_cards(rng)
        if cards in seen:
            continue
        seen.add(cards)
        deals.append(
            DealSpec(
                dealer=rng.randrange(4),
                vul=rng.randrange(4),
                trump=rng.randrange(5),
                first=rng.randrange(4),
                cards=cards,
            )
        )
    return deals


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


def calc_tables_batched(
    cards: list[str],
    *,
    calc_fn=None,
    batch_size: int = MAX_TABLES_PER_BATCH,
) -> list[dict]:
    """Compute DD tables for ``cards``, chunked to ``batch_size``."""
    if batch_size <= 0:
        raise ValueError(f"batch_size must be positive, got {batch_size}")
    if not cards:
        return []

    if calc_fn is None:
        calc_fn = calc_all_tables_pbn

    tables: list[dict] = []
    for start in range(0, len(cards), batch_size):
        chunk = cards[start : start + batch_size]
        batch = calc_fn(chunk)
        chunk_tables = batch["tables"]
        if len(chunk_tables) != len(chunk):
            raise RuntimeError(
                f"calc_all_tables_pbn returned {len(chunk_tables)} tables "
                f"for {len(chunk)} deals"
            )
        tables.extend(chunk_tables)
    return tables


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
    tricks = solved["tricks"][: solved["number"]]
    body = " ".join(str(t) for t in tricks)
    return f'TRACE {solved["number"]} {body} \n'


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
        raise ValueError(f"remain cards must start with N:/E:/S:/W:: {cards!r}")
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


def fill_hand_list_text(text: str) -> str:
    """Return ``text`` with TABLE/PAR/PAR2/PLAY/TRACE filled from DDS."""
    lines = text.splitlines(keepends=True)

    deals: list[tuple[int, int, int, int, str]] = []
    for line in lines:
        if line.startswith("PBN "):
            deals.append(_parse_pbn_line(line))

    if not deals:
        return text

    cards = [c for (_dealer, _vul, _trump, _first, c) in deals]
    tables = calc_tables_batched(cards)
    if len(tables) != len(deals):
        raise RuntimeError(
            f"calc_tables_batched returned {len(tables)} tables "
            f"for {len(deals)} deals"
        )

    filled: dict[str, list[str]] = {
        "TABLE": [],
        "PAR": [],
        "PAR2": [],
        "PLAY": [],
        "TRACE": [],
    }
    for (dealer, vul, trump, first, remain), table in zip(deals, tables):
        table_dict = {"res_table": table["res_table"]}
        filled["TABLE"].append(format_table_line(table["res_table"]))
        filled["PAR"].append(format_par_line(par(table_dict, vul)))
        filled["PAR2"].append(
            format_par2_line(dealer_par(table_dict, dealer, vul))
        )

        play = generate_dd_play(remain, trump=trump, first=first)
        filled["PLAY"].append(format_play_line(play))
        solved = analyse_play_pbn(remain, play=play, trump=trump, first=first)
        filled["TRACE"].append(format_trace_line(solved))

    cursors = {key: 0 for key in filled}
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

        idx = cursors[key]
        if idx >= len(filled[key]):
            raise RuntimeError(f"more {key} lines than PBN deals")
        out.append(filled[key][idx])
        cursors[key] = idx + 1

    for key, idx in cursors.items():
        if idx != len(filled[key]):
            raise RuntimeError(
                f"expected {len(filled[key])} {key} lines, found {idx}"
            )

    return "".join(out)


def build_hand_list(
    deals: Sequence[DealSpec],
    futs: Sequence[dict[str, Any]],
    *,
    fill_fn: Callable[[str], str] | None = None,
) -> str:
    """Build a hand list, then fill TABLE/PAR/PAR2/PLAY/TRACE.

    By default ``fill_fn`` is ``fill_hand_list_text``.
    Pass an identity (or other) function in tests to skip DDS fill work.
    """
    if len(deals) != len(futs):
        raise ValueError("deals and futs length mismatch")
    parts = [f"NUMBER {len(deals)} \n"]
    for deal, fut in zip(deals, futs):
        parts.append(format_deal_block(deal, fut))
    text = "".join(parts)
    if text and not text.endswith("\n"):
        text += "\n"

    if fill_fn is None:
        fill_fn = fill_hand_list_text

    return fill_fn(text)


def solve_futs(
    deals: Sequence[DealSpec],
    *,
    solve_batch: Callable[..., list[dict[str, Any]]] | None = None,
    max_threads: int = 0,
) -> list[dict[str, Any]]:
    """Solve deals in batches matching ``dtest`` solve parameters."""
    if solve_batch is None:
        from dds3 import solve_all_boards_pbn as solve_batch  # type: ignore

    futs: list[dict[str, Any]] = []
    for start in range(0, len(deals), _BATCH):
        chunk = deals[start : start + _BATCH]
        boards = [
            {
                "remain_cards": d.cards,
                "trump": d.trump,
                "first": d.first,
                "target": -1,
                "solutions": 3,
                "mode": 1,
            }
            for d in chunk
        ]
        futs.extend(solve_batch(boards, max_threads=max_threads))
    return futs


def _resolve_output_path(path: Path) -> Path:
    if path.is_absolute():
        return path
    working = os.environ.get("BUILD_WORKING_DIRECTORY")
    if working:
        return Path(working) / path
    return path


def write_hand_list_output(
    text: str,
    *,
    output: Path | None,
    stdout: TextIO | None = None,
) -> None:
    """Write ``text`` to ``output`` or to stdout when ``output`` is None."""
    if output is None:
        out_stream = sys.stdout if stdout is None else stdout
        out_stream.write(text)
        return

    path = _resolve_output_path(output)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def _build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument(
        "-n",
        "--count",
        type=int,
        default=10,
        help="Number of unique deals (default: 10)",
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
    p.add_argument(
        "--max-threads",
        type=int,
        default=0,
        help="DDS worker threads for solving (0 = auto)",
    )
    return p


def _parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    return _build_parser().parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args_list = sys.argv[1:] if argv is None else list(argv)
    if not args_list:
        _build_parser().print_help()
        return 2
    args = _parse_args(args_list)
    deals = generate_unique_deals(args.count, seed=args.seed)
    print(f"Generated {len(deals)} unique deals; solving FUT…", file=sys.stderr)
    futs = solve_futs(deals, max_threads=args.max_threads)
    print("Filling TABLE/PAR/PAR2/PLAY/TRACE…", file=sys.stderr)
    text = build_hand_list(deals, futs)
    write_hand_list_output(text, output=args.output)
    if args.output is None:
        print(f"Wrote {len(deals)} deals -> stdout", file=sys.stderr)
    else:
        out = _resolve_output_path(args.output)
        print(f"Wrote {len(deals)} deals -> {out}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
