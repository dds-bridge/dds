#!/usr/bin/env python3
import sys
import json

def filter_jsonl(in_path, out_path, board_id):
    with open(in_path, 'r', encoding='utf-8', errors='ignore') as inf, open(out_path, 'w', encoding='utf-8') as outf:
        for line in inf:
            line = line.strip()
            if not line:
                continue
            try:
                obj = json.loads(line)
            except Exception:
                # Skip lines that aren't standalone JSON objects
                # Try to decode fragments using JSONDecoder not necessary here for filtered smaller set
                continue
            if not isinstance(obj, dict):
                continue
            if str(obj.get('board_id','')) == str(board_id):
                outf.write(json.dumps(obj) + '\n')

if __name__ == '__main__':
    if len(sys.argv) < 4:
        print('Usage: filter_jsonl_by_board.py input.jsonl out.jsonl board_id')
        sys.exit(2)
    filter_jsonl(sys.argv[1], sys.argv[2], sys.argv[3])
