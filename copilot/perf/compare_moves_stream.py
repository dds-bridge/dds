#!/usr/bin/env python3
import sys

def read_header(f):
    hdr = f.readline()
    if not hdr:
        return []
    return hdr.strip().split('\t')

def parse_key(fields):
    # key: board_id, stage, currTrick, currHand, leadHand, leadSuit, idx
    # include the 16 rankInSuit mask columns and 4 aggr columns for exact position matching
    base = [fields[0], fields[2], fields[3], fields[4], fields[5], fields[6], fields[8]]
    # mask columns begin after weight; according to normalize_moves_log.py header they start at index 13
    masks = []
    # indices 13..28 -> rankInSuit_h0s0 .. rankInSuit_h3s3
    for i in range(13, 29):
        if i < len(fields):
            masks.append(fields[i])
        else:
            masks.append('')
    # aggr indices 29..32
    ag = []
    for i in range(29, 33):
        if i < len(fields):
            ag.append(fields[i])
        else:
            ag.append('')
    return tuple(base + masks + ag)

def group_by_key(f):
    """Generator yielding (key, [fields_lines]) grouped by key for sorted TSV (no header)."""
    cur_key = None
    cur_group = []
    for line in f:
        line = line.rstrip('\n')
        if not line:
            continue
        fields = line.split('\t')
        key = parse_key(fields)
        if cur_key is None:
            cur_key = key
            cur_group = [fields]
            continue
        if key == cur_key:
            cur_group.append(fields)
        else:
            yield (cur_key, cur_group)
            cur_key = key
            cur_group = [fields]
    if cur_key is not None:
        yield (cur_key, cur_group)

def compare(sorted_legacy_path, sorted_new_path, out_summary):
    with open(sorted_legacy_path, 'r', encoding='utf-8', errors='ignore') as lf, open(sorted_new_path, 'r', encoding='utf-8', errors='ignore') as nf:
        lhdr = read_header(lf)
        nhdr = read_header(nf)
        # ensure compatible headers
        # weight column index
        try:
            l_widx = lhdr.index('weight')
        except ValueError:
            l_widx = None
        try:
            n_widx = nhdr.index('weight')
        except ValueError:
            n_widx = None

        lg = group_by_key(lf)
        ng = group_by_key(nf)
        try:
            lk, lrows = next(lg)
        except StopIteration:
            lk, lrows = None, None
        try:
            nk, nrows = next(ng)
        except StopIteration:
            nk, nrows = None, None

        only_legacy = []
        only_new = []
        weight_diffs = []

        while lk is not None or nk is not None:
            if nk is None or (lk is not None and lk < nk):
                only_legacy.append((lk, lrows))
                try:
                    lk, lrows = next(lg)
                except StopIteration:
                    lk, lrows = None, None
                continue
            if lk is None or (nk is not None and nk < lk):
                only_new.append((nk, nrows))
                try:
                    nk, nrows = next(ng)
                except StopIteration:
                    nk, nrows = None, None
                continue
            # keys equal
            # pick first row from each group to compare weights
            lw = ''
            nw = ''
            if l_widx is not None and len(lrows[0]) > l_widx:
                lw = lrows[0][l_widx]
            if n_widx is not None and len(nrows[0]) > n_widx:
                nw = nrows[0][n_widx]
            try:
                li = int(lw) if lw != '' else None
            except Exception:
                li = None
            try:
                ni = int(nw) if nw != '' else None
            except Exception:
                ni = None
            if li != ni:
                weight_diffs.append((lk, li, ni))
            try:
                lk, lrows = next(lg)
            except StopIteration:
                lk, lrows = None, None
            try:
                nk, nrows = next(ng)
            except StopIteration:
                nk, nrows = None, None

    with open(out_summary, 'w', encoding='utf-8') as out:
        out.write('# Discrepancy report (streaming)\n')
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
        print('Usage: compare_moves_stream.py sorted_legacy.tsv sorted_new.tsv out_summary.txt')
        sys.exit(2)
    compare(sys.argv[1], sys.argv[2], sys.argv[3])
