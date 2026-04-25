#!/usr/bin/env python3
from pathlib import Path
import re
import sys

if len(sys.argv) not in (2, 3):
    print("Usage: coverage_summary.py <build_coverage_dir> [minimum_percent]", file=sys.stderr)
    sys.exit(1)

coverage_dir = Path(sys.argv[1])
minimum_percent = None
if len(sys.argv) == 3:
    try:
        minimum_percent = float(sys.argv[2])
    except ValueError:
        print(f"Invalid minimum coverage percentage: {sys.argv[2]}", file=sys.stderr)
        sys.exit(1)

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
    overall = total / count
    print(f"Overall project coverage: {overall:.2f}% ({count} lines)")

    print("\nLowest covered production files:")
    for path, (pct, lines) in sorted(best.items(), key=lambda item: item[1][0])[:10]:
        rel = Path(path).relative_to(project_root)
        print(f"  {pct:6.2f}%  {lines:4d} lines  {rel}")

    if minimum_percent is not None and overall < minimum_percent:
        print(
            f"\nCoverage gate failed: {overall:.2f}% is below required {minimum_percent:.2f}%",
            file=sys.stderr,
        )
        sys.exit(2)

    if minimum_percent is not None:
        print(f"\nCoverage gate passed: {overall:.2f}% >= {minimum_percent:.2f}%")
else:
    print("Overall project coverage: unavailable")
    if minimum_percent is not None:
        sys.exit(2)
