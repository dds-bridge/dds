#!/usr/bin/env python3
import csv
import sys
from collections import defaultdict

def read_tsv(path):
    rows = []
    with open(path, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f, delimiter='\t')
        for r in reader:
            rows.append(r)
    return rows

def key_of(r):
    # Unique key for a move occurrence
    return (r['board_id'], r['stage'], r['currTrick'], r['currHand'], r['leadHand'], r['leadSuit'], r['idx'])

def compare(legacy_tsv, new_tsv, out_summary):
    legacy = read_tsv(legacy_tsv)
    new = read_tsv(new_tsv)

    L = defaultdict(list)
    N = defaultdict(list)
    for r in legacy: L[key_of(r)].append(r)
    for r in new: N[key_of(r)].append(r)

    only_legacy = []
    only_new = []
    weight_diffs = []

    all_keys = set(L.keys()) | set(N.keys())
    for k in sorted(all_keys):
        l = L.get(k, [])
        n = N.get(k, [])
        if not l:
            only_new.append((k, n))
            continue
        if not n:
            only_legacy.append((k, l))
            continue
        # compare weights if both exist (pick first if duplicates)
        lw = int(l[0]['weight'])
        nw = int(n[0]['weight'])
        if lw != nw:
            weight_diffs.append((k, lw, nw))

    with open(out_summary, 'w', encoding='utf-8') as out:
        out.write('# Discrepancy report\n')
        out.write(f'legacy_only_count: {len(only_legacy)}\n')
        out.write(f'new_only_count: {len(only_new)}\n')
        out.write(f'weight_diffs_count: {len(weight_diffs)}\n')
        out.write('\n')
        if weight_diffs:
            out.write('Weight differences (board_id,stage,currTrick,currHand,leadHand,leadSuit,idx): legacy->new\n')
            for k,lw,nw in weight_diffs[:200]:
                out.write(f"{k}: {lw} -> {nw}\n")
        if only_legacy:
            out.write('\nLegacy-only examples:\n')
            for k,rows in only_legacy[:20]:
                out.write(str(k) + '\n')
        if only_new:
            out.write('\nNew-only examples:\n')
            for k,rows in only_new[:20]:
                out.write(str(k) + '\n')

if __name__ == '__main__':
    if len(sys.argv) < 4:
        print('Usage: compare_moves.py legacy.tsv new.tsv out_summary.txt')
        sys.exit(2)
    compare(sys.argv[1], sys.argv[2], sys.argv[3])
