#!/usr/bin/env python3
import json
import sys

"""
Parse JSONL MOVES_LOG entries and write a TSV with fields suitable for diffing.

Usage: normalize_moves_log.py <input-log> <output-tsv>
"""

def normalize_jsonl(in_path, out_path):
    with open(in_path, 'r', encoding='utf-8', errors='ignore') as f, open(out_path, 'w', encoding='utf-8') as out:
        out.write('board_id\tvariant\tstage\tcurrTrick\tcurrHand\tleadHand\tleadSuit\tnumMoves\tidx\tsuit\trank\tseq\tweight\n')
        decoder = json.JSONDecoder()
        for line in f:
            line = line.strip()
            if not line:
                continue
            idx = 0
            L = len(line)
            # support multiple JSON objects concatenated on the same line
            while idx < L:
                try:
                    obj, end = decoder.raw_decode(line, idx)
                except Exception:
                    break
                idx = end
                # skip whitespace/separators
                while idx < L and line[idx].isspace():
                    idx += 1

                # only handle dict objects with expected structure
                if not isinstance(obj, dict):
                    continue

                board = obj.get('board_id','')
                variant = obj.get('variant','')
                stage = obj.get('stage','')
                currTrick = obj.get('currTrick','')
                currHand = obj.get('currHand','')
                leadHand = obj.get('leadHand','')
                leadSuit = obj.get('leadSuit','')
                numMoves = obj.get('numMoves','')
                for mv in obj.get('moves',[]):
                    out.write(f"{board}\t{variant}\t{stage}\t{currTrick}\t{currHand}\t{leadHand}\t{leadSuit}\t{numMoves}\t{mv.get('idx','')}\t{mv.get('suit','')}\t{mv.get('rank','')}\t{mv.get('seq','')}\t{mv.get('w','')}\n")


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print('Usage: normalize_moves_log.py <input-log> <output-tsv>')
        sys.exit(2)
    normalize_jsonl(sys.argv[1], sys.argv[2])

