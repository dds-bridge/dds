#!/usr/bin/env python3
import csv
import sys
import os
from collections import defaultdict

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

def compute_ranking(legacy_rows, new_rows):
    L = defaultdict(list)
    N = defaultdict(list)
    for r in legacy_rows: L[key_of(r)].append(r)
    for r in new_rows: N[key_of(r)].append(r)
    per_board = defaultdict(lambda: {'only_legacy':0,'only_new':0,'weight_diffs':0,'pbn':''})
    for r in legacy_rows:
        if 'board_pbn' in r and r['board_pbn']:
            per_board[board_key(r)]['pbn'] = r['board_pbn']
    for r in new_rows:
        if 'board_pbn' in r and r['board_pbn']:
            per_board[board_key(r)]['pbn'] = r['board_pbn']
    all_keys = set(L.keys()) | set(N.keys())
    for k in all_keys:
        bid = k[0]
        l = L.get(k, [])
        n = N.get(k, [])
        if not l:
            per_board[bid]['only_new'] += len(n)
            continue
        if not n:
            per_board[bid]['only_legacy'] += len(l)
            continue
        lw = int(l[0]['weight'])
        nw = int(n[0]['weight'])
        if lw != nw:
            per_board[bid]['weight_diffs'] += 1
    ranking = []
    for bid,data in per_board.items():
        total = data['only_legacy'] + data['only_new'] + data['weight_diffs']
        ranking.append((bid, total, data.get('pbn','')))
    ranking.sort(key=lambda x: (-x[1], int(x[0]) if x[0].isdigit() else x[0]))
    return ranking

def gather_board_details(legacy_rows, new_rows, bid):
    L = defaultdict(list)
    N = defaultdict(list)
    for r in legacy_rows: 
        if r['board_id'] == bid:
            L[(r['stage'], r['currTrick'], r['currHand'], r['leadHand'], r['leadSuit'], r['idx'])].append(r)
    for r in new_rows:
        if r['board_id'] == bid:
            N[(r['stage'], r['currTrick'], r['currHand'], r['leadHand'], r['leadSuit'], r['idx'])].append(r)
    keys = sorted(set(list(L.keys()) + list(N.keys())))
    items = []
    for k in keys:
        llist = L.get(k, [])
        nlist = N.get(k, [])
        items.append((k, llist, nlist))
    return items

def render_board_html(bid, pbn, items, out_path):
    with open(out_path, 'w', encoding='utf-8') as out:
        out.write('<!doctype html>\n<html><head><meta charset="utf-8">\n')
        out.write('<title>Board '+str(bid)+' — Move diffs</title>\n')
        out.write('<style>body{font-family: Arial, sans-serif; padding:20px} .cols{display:flex;gap:20px} .col{flex:1} table{border-collapse:collapse;width:100%} th,td{border:1px solid #ddd;padding:6px;font-size:12px} th{background:#f2f2f2} .diff{background:#ffe6e6} .ok{background:#e6ffe6} .moves{font-family:monospace;font-size:12px}</style>\n')
        out.write('</head><body>\n')
        out.write(f'<h1>Board {bid}</h1>\n')
        out.write('<h3>PBN</h3>\n<pre>'+ (pbn if pbn else '(none)') + '</pre>\n')
        out.write('<div class="cols">\n<div class="col">\n')
        out.write('<h2>Legacy</h2>\n')
        out.write('<table><tr><th>stage</th><th>currTrick</th><th>currHand</th><th>leadHand</th><th>leadSuit</th><th>idx</th><th>move(s)</th><th>weight</th></tr>\n')
        # legacy rows
        for k, llist, nlist in items:
            for r in llist:
                moves = f"s:{r.get('suit','')} r:{r.get('rank','')} seq:{r.get('seq','')}"
                out.write('<tr>')
                out.write(f'<td>{r.get("stage","")}</td><td>{r.get("currTrick","")}</td><td>{r.get("currHand","")}</td><td>{r.get("leadHand","")}</td><td>{r.get("leadSuit","")}</td><td>{r.get("idx","")}</td>')
                out.write(f'<td class="moves">{moves}</td><td>{r.get("weight","")}</td>')
                out.write('</tr>\n')
        out.write('</table>\n</div>\n')

        out.write('<div class="col">\n<h2>New</h2>\n')
        out.write('<table><tr><th>stage</th><th>currTrick</th><th>currHand</th><th>leadHand</th><th>leadSuit</th><th>idx</th><th>move(s)</th><th>weight</th></tr>\n')
        for k, llist, nlist in items:
            for r in nlist:
                moves = f"s:{r.get('suit','')} r:{r.get('rank','')} seq:{r.get('seq','')}"
                out.write('<tr>')
                out.write(f'<td>{r.get("stage","")}</td><td>{r.get("currTrick","")}</td><td>{r.get("currHand","")}</td><td>{r.get("leadHand","")}</td><td>{r.get("leadSuit","")}</td><td>{r.get("idx","")}</td>')
                out.write(f'<td class="moves">{moves}</td><td>{r.get("weight","")}</td>')
                out.write('</tr>\n')
        out.write('</table>\n</div>\n</div>\n')
        out.write('<p><a href="index.html">Back to index</a></p>\n')
        out.write('</body></html>')

def main():
    if len(sys.argv) < 5:
        print('Usage: generate_board_html.py legacy.tsv new.tsv ranking_or_outdir out_dir [top_n]')
        print('If ranking_or_outdir is an existing ranking file, it will be used; otherwise it is treated as out_dir and ranking computed.')
        sys.exit(2)
    legacy_tsv = sys.argv[1]
    new_tsv = sys.argv[2]
    ranking_arg = sys.argv[3]
    out_dir = sys.argv[4]
    top_n = int(sys.argv[5]) if len(sys.argv) > 5 else 5

    legacy = read_tsv(legacy_tsv)
    new = read_tsv(new_tsv)

    if os.path.isfile(ranking_arg):
        # read ranking file expecting board_id	...
        ranking = []
        with open(ranking_arg,'r',encoding='utf-8') as f:
            header = f.readline()
            for line in f:
                parts = line.strip().split('\t')
                if not parts: continue
                bid = parts[0]
                pbn = parts[3] if len(parts) > 3 else ''
                ranking.append((bid, int(parts[1]) if len(parts)>1 and parts[1].isdigit() else 0, pbn))
    else:
        ranking = compute_ranking(legacy, new)

    os.makedirs(out_dir, exist_ok=True)
    index_path = os.path.join(out_dir, 'index.html')
    # prepare index
    with open(index_path, 'w', encoding='utf-8') as idx:
        idx.write('<!doctype html><html><head><meta charset="utf-8"><title>Top boards</title></head><body>')
        idx.write('<h1>Top boards</h1><ul>')
        for bid, total, pbn in ranking[:top_n]:
            fname = f'board_{bid}.html'
            idx.write(f'<li><a href="{fname}">Board {bid} — diffs: {total}</a></li>')
        idx.write('</ul></body></html>')

    # render top boards
    for bid, total, pbn in ranking[:top_n]:
        items = gather_board_details(legacy, new, bid)
        out_path = os.path.join(out_dir, f'board_{bid}.html')
        render_board_html(bid, pbn, items, out_path)

    print(f'Wrote HTML reports to {out_dir} (top {top_n})')

if __name__ == '__main__':
    main()
