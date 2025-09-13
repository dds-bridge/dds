#!/usr/bin/env python3
import sys

def sample(in_path, out_path, n):
    n = int(n)
    cnt = 0
    with open(in_path, 'r', encoding='utf-8', errors='ignore') as inf, open(out_path, 'w', encoding='utf-8') as outf:
        for line in inf:
            if cnt >= n:
                break
            outf.write(line)
            cnt += 1

if __name__ == '__main__':
    if len(sys.argv) < 4:
        print('Usage: sample_jsonl.py input.jsonl out.jsonl N')
        sys.exit(2)
    sample(sys.argv[1], sys.argv[2], sys.argv[3])
