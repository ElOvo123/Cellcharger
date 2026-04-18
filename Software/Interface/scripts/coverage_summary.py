#!/usr/bin/env python3
from pathlib import Path
import re
import sys

if len(sys.argv) != 2:
    print("Usage: coverage_summary.py <build_coverage_dir>", file=sys.stderr)
    sys.exit(1)

coverage_dir = Path(sys.argv[1])
summary_file = coverage_dir / 'coverage-summary.txt'
if not summary_file.exists():
    print(f"Coverage summary file not found: {summary_file}", file=sys.stderr)
    sys.exit(1)

project_root = coverage_dir.resolve().parent
summary = summary_file.read_text().splitlines()
cur = None
best = {}
for line in summary:
    if line.startswith("File '"):
        cur = line[6:-1]
        continue
    if cur and line.startswith("No executable lines"):
        cur = None
        continue
    if cur and line.startswith("Lines executed:"):
        match = re.match(r"Lines executed:([0-9.]+)% of ([0-9]+)", line)
        if match:
            pct = float(match.group(1))
            lines = int(match.group(2))
            if cur.startswith(str(project_root)) and cur.endswith('.cpp') and '/tests/' not in cur and '/build-' not in cur:
                previous = best.get(cur)
                if previous is None or pct > previous[0]:
                    best[cur] = (pct, lines)
        cur = None

count = sum(lines for _, lines in best.values())
total = sum(pct * lines for pct, lines in best.values())
if count:
    print(f"Overall project coverage: {total / count:.2f}% ({count} lines)")
else:
    print("Overall project coverage: unavailable")
