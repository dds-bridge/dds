#!/usr/bin/env python3
import json
import sys

"""
Parse JSONL MOVES_LOG entries and write a TSV with fields suitable for diffing.

Usage: normalize_moves_log.py <input-log> <output-tsv>
"""

def normalize_jsonl(in_path, out_path):
    with open(in_path, 'r', encoding='utf-8', errors='ignore') as f, open(out_path, 'w', encoding='utf-8') as out:
        # header: base fields + 16 rankInSuit mask columns + 4 aggr columns
        mask_cols = []
        for h in range(4):
            for s in range(4):
                mask_cols.append(f'rankInSuit_h{h}s{s}')
        aggr_cols = [f'aggr_s{s}' for s in range(4)]
        out.write('board_id\tvariant\tstage\tcurrTrick\tcurrHand\tleadHand\tleadSuit\tnumMoves\tidx\tsuit\trank\tseq\tweight')
        for c in mask_cols: out.write('\t' + c)
        for c in aggr_cols: out.write('\t' + c)
        out.write('\n')
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
                # canonicalize common typos/variants between runs
                if stage == 'post-sor':
                    stage = 'post-sort'
                currTrick = obj.get('currTrick','')
                currHand = obj.get('currHand','')
                leadHand = obj.get('leadHand','')
                leadSuit = obj.get('leadSuit','')
                numMoves = obj.get('numMoves','')
                for mv in obj.get('moves',[]):
                    # weight may be in 'weight' or legacy 'w'
                    weight = mv.get('weight', mv.get('w', ''))
                    # flatten rankInSuit (4x4) if present
                    ris = mv.get('rankInSuit', [])
                    ris_flat = []
                    for h in range(4):
                        row = []
                        if h < len(ris) and isinstance(ris[h], list):
                            row = ris[h]
                        for s in range(4):
                            if s < len(row):
                                ris_flat.append(str(row[s]))
                            else:
                                ris_flat.append('')
                    ag = mv.get('aggr', [])
                    ag_vals = []
                    for s in range(4):
                        if s < len(ag):
                            ag_vals.append(str(ag[s]))
                        else:
                            ag_vals.append('')

                    out.write(f"{board}\t{variant}\t{stage}\t{currTrick}\t{currHand}\t{leadHand}\t{leadSuit}\t{numMoves}\t{mv.get('idx','')}\t{mv.get('suit','')}\t{mv.get('rank','')}\t{mv.get('seq','')}\t{weight}")
                    for v in ris_flat: out.write('\t' + v)
                    for v in ag_vals: out.write('\t' + v)
                    out.write('\n')


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print('Usage: normalize_moves_log.py <input-log> <output-tsv>')
        sys.exit(2)
    normalize_jsonl(sys.argv[1], sys.argv[2])

