#!/usr/bin/env python3
import csv
import sys
from collections import defaultdict
import os

def read_tsv(path):
    rows = []
    with open(path, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f, delimiter='\t')
        for r in reader:
            rows.append(r)
    return rows

def key_of(r):
    return (r['board_id'], r['stage'], r['currTrick'], r['currHand'], r['leadHand'], r['leadSuit'], r['idx'])

def board_key(r):
    return r['board_id']

def generate(legacy_tsv, new_tsv, out_dir, top_n=5):
    legacy = read_tsv(legacy_tsv)
    new = read_tsv(new_tsv)

    L = defaultdict(list)
    N = defaultdict(list)
    for r in legacy: L[key_of(r)].append(r)
    for r in new: N[key_of(r)].append(r)

    # collect per-board diffs
    per_board = defaultdict(lambda: {'only_legacy':[], 'only_new':[], 'weight_diffs':[],'pbn':''})

    # capture pbn if present
    for r in legacy:
        if 'board_pbn' in r and r['board_pbn']:
            per_board[board_key(r)]['pbn'] = r['board_pbn']
    for r in new:
        if 'board_pbn' in r and r['board_pbn']:
            per_board[board_key(r)]['pbn'] = r['board_pbn']

    all_keys = set(L.keys()) | set(N.keys())
    for k in all_keys:
        bid = k[0]
        l = L.get(k, [])
        n = N.get(k, [])
        if not l:
            per_board[bid]['only_new'].append((k,n))
            continue
        if not n:
            per_board[bid]['only_legacy'].append((k,l))
            continue
        lw = int(l[0]['weight'])
        nw = int(n[0]['weight'])
        if lw != nw:
            per_board[bid]['weight_diffs'].append((k,lw,nw))

    # rank boards: primary by total diffs, secondary by max abs weight delta
    ranking = []
    for bid, data in per_board.items():
        total = len(data['only_legacy']) + len(data['only_new']) + len(data['weight_diffs'])
        max_diff = 0
        for _k,lw,nw in data['weight_diffs']:
            max_diff = max(max_diff, abs(lw-nw))
        ranking.append((bid, total, max_diff, data))

    ranking.sort(key=lambda x: ( -x[1], -x[2], int(x[0]) if x[0].isdigit() else x[0]))

    os.makedirs(out_dir, exist_ok=True)
    summary_path = os.path.join(out_dir, 'per_board_ranking.txt')
    with open(summary_path, 'w', encoding='utf-8') as out:
        out.write('board_id\ttotal_diffs\tmax_abs_weight_diff\tpbn\n')
        for bid,total,maxdiff,_data in ranking[:200]:
            out.write(f"{bid}\t{total}\t{maxdiff}\t{_data.get('pbn','')}\n")

    # write side-by-side reports for top N
    for bid,total,maxdiff,data in ranking[:top_n]:
        fn = os.path.join(out_dir, f'board_{bid}_report.txt')
        with open(fn, 'w', encoding='utf-8') as out:
            out.write(f"Board {bid}  — total diffs: {total}, max_abs_weight_diff: {maxdiff}\n")
            out.write('\nPBN:\n')
            out.write(data.get('pbn','(none)') + '\n\n')

            out.write('Weight diffs (stage,currTrick,currHand,leadHand,leadSuit,idx): legacy -> new\n')
            for k,lw,nw in data['weight_diffs']:
                out.write(f"{k[1:]}	{lw} -> {nw}\n")

            out.write('\nLegacy-only moves (examples):\n')
            for k,rows in data['only_legacy'][:200]:
                out.write(str(k) + '\n')
                for r in rows[:10]:
                    out.write('\t' + '\t'.join([r.get('variant',''), r.get('stage',''), r.get('currTrick',''), r.get('currHand',''), r.get('leadHand',''), r.get('leadSuit',''), r.get('idx',''), r.get('suit',''), r.get('rank',''), r.get('seq',''), r.get('weight','')]) + '\n')

            out.write('\nNew-only moves (examples):\n')
            for k,rows in data['only_new'][:200]:
                out.write(str(k) + '\n')
                for r in rows[:10]:
                    out.write('\t' + '\t'.join([r.get('variant',''), r.get('stage',''), r.get('currTrick',''), r.get('currHand',''), r.get('leadHand',''), r.get('leadSuit',''), r.get('idx',''), r.get('suit',''), r.get('rank',''), r.get('seq',''), r.get('weight','')]) + '\n')

    print(f'Wrote per-board reports to {out_dir} (top {top_n})')

if __name__ == '__main__':
    if len(sys.argv) < 4:
        print('Usage: generate_board_reports.py legacy.tsv new.tsv out_dir [top_n]')
        sys.exit(2)
    top_n = int(sys.argv[4]) if len(sys.argv) > 4 else 5
    generate(sys.argv[1], sys.argv[2], sys.argv[3], top_n)
