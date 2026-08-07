#!/usr/bin/env python3
"""
ddx3216_fw_analyze.py

Quick recon tool for Behringer DDX3216 firmware/ROM dumps (.bex, .bin, EPROM images).

What it does:
  1. Reports file size + header/tail bytes.
  2. Computes block entropy (4KB windows) to flag likely code vs data vs
     padding/blank regions (useful before pointing a disassembler at it).
  3. Extracts embedded ASCII strings.
  4. Applies a light "de-vowel" heuristic: many embedded/POST-era firmware
     strip vowels from debug strings to save ROM space (confirmed in the
     DDX3216 flash images -- e.g. "GNRLERR!!" = "GENERAL ERR!!"). This just
     flags candidates and shows them un-mangled; it does NOT attempt full
     reconstruction (too many false positives for that).

Usage:
    python3 ddx3216_fw_analyze.py <file1.bin> [file2.bin ...]
    python3 ddx3216_fw_analyze.py --diff <fileA.bin> <fileB.bin>   # byte-diff two same-size images
"""

import sys
import re
import math
import collections


def entropy(chunk: bytes) -> float:
    if not chunk:
        return 0.0
    counts = collections.Counter(chunk)
    n = len(chunk)
    return -sum((v / n) * math.log2(v / n) for v in counts.values())


def block_entropy_report(data: bytes, block_size: int = 4096, max_blocks: int = 64):
    print(f"  overall entropy: {entropy(data):.3f} bits/byte  (8.0 = random/compressed/encrypted, "
          f"~4-5 = text, ~5.5-6.5 = typical machine code, near 0 = padding/blank)")
    step = max(block_size, len(data) // max_blocks) if len(data) > block_size * max_blocks else block_size
    print(f"  block entropy (window={step} bytes):")
    for i in range(0, len(data), step):
        chunk = data[i:i + step]
        e = entropy(chunk)
        bar = "#" * int(e / 8 * 40)
        print(f"    0x{i:07x}  {e:5.2f}  {bar}")


def extract_strings(data: bytes, min_len: int = 5):
    return re.findall(rb"[\x20-\x7e]{%d,}" % min_len, data)


def wordlike(strings):
    return [s for s in strings if re.search(rb"[A-Za-z]{3,}", s)]


VOWELS = set(b"AEIOUaeiou")


def looks_devoweled(s: bytes) -> bool:
    """Heuristic: mostly consonants/digits/punct, few or no vowels, len>=5."""
    letters = [c for c in s if chr(c).isalpha()]
    if len(letters) < 4:
        return False
    vowel_count = sum(1 for c in letters if c in VOWELS)
    return (vowel_count / len(letters)) < 0.15


def analyze_file(path: str):
    with open(path, "rb") as f:
        data = f.read()

    print(f"\n=== {path} ===")
    print(f"  size: {len(data)} bytes")
    print(f"  head: {data[:32].hex()}")
    print(f"  tail: {data[-32:].hex()}")

    block_entropy_report(data)

    strings = extract_strings(data)
    words = wordlike(strings)
    print(f"  strings found: {len(strings)} total, {len(words)} word-like")

    devoweled = [s for s in words if looks_devoweled(s)]
    print(f"  likely de-voweled debug/POST strings ({len(devoweled)}):")
    seen = set()
    shown = 0
    for s in devoweled:
        if s in seen:
            continue
        seen.add(s)
        print(f"    {s.decode('ascii', 'replace')}")
        shown += 1
        if shown >= 60:
            print("    ... (truncated, raise the limit in-script if you need more)")
            break


def diff_files(path_a: str, path_b: str):
    a = open(path_a, "rb").read()
    b = open(path_b, "rb").read()
    print(f"\n=== diff {path_a} vs {path_b} ===")
    if len(a) != len(b):
        print(f"  size mismatch: {len(a)} vs {len(b)} bytes -- diffing up to shorter length")
    n = min(len(a), len(b))
    diffs = [i for i in range(n) if a[i] != b[i]]
    print(f"  {len(diffs)} differing bytes out of {n} ({100*len(diffs)/n:.2f}%)")
    if not diffs:
        return
    # collapse into contiguous runs for readability
    runs = []
    start = diffs[0]
    prev = diffs[0]
    for i in diffs[1:]:
        if i != prev + 1:
            runs.append((start, prev))
            start = i
        prev = i
    runs.append((start, prev))
    print(f"  {len(runs)} contiguous diff regions; first 30:")
    for s, e in runs[:30]:
        print(f"    0x{s:07x} - 0x{e:07x}  ({e - s + 1} bytes)")


if __name__ == "__main__":
    args = sys.argv[1:]
    if not args:
        print(__doc__)
        sys.exit(1)
    if args[0] == "--diff":
        if len(args) != 3:
            print("Usage: ddx3216_fw_analyze.py --diff <fileA> <fileB>")
            sys.exit(1)
        diff_files(args[1], args[2])
    else:
        for p in args:
            analyze_file(p)
