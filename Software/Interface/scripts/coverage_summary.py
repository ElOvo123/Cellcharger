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


def is_production_cpp(path):
    return path.startswith(str(project_root)) and path.endswith('.cpp') and '/tests/' not in path and '/build-' not in path


def parse_gcov_line_count(raw_count):
    raw_count = raw_count.strip()
    if raw_count == '-':
        return None
    if raw_count in ('#####', '====='):
        return 0

    raw_count = raw_count.rstrip('*')
    try:
        return int(raw_count)
    except ValueError:
        return None


def aggregate_line_coverage(gcov_dir):
    line_hits_by_file = {}

    for gcov_file in gcov_dir.glob('*.gcov'):
        source = None
        excluded_region = False
        for line in gcov_file.read_text(errors='replace').splitlines():
            parts = line.split(':', 2)
            if len(parts) != 3:
                continue

            raw_count, raw_line_number, source_text = parts
            raw_line_number = raw_line_number.strip()
            if raw_line_number == '0' and source_text.startswith('Source:'):
                source = source_text[len('Source:'):].strip()
                if not is_production_cpp(source):
                    source = None
                continue

            if source is None:
                continue

            if 'LCOV_EXCL_START' in source_text:
                excluded_region = True
                continue

            if 'LCOV_EXCL_STOP' in source_text:
                excluded_region = False
                continue

            if excluded_region or 'LCOV_EXCL_LINE' in source_text:
                continue

            count = parse_gcov_line_count(raw_count)
            if count is None:
                continue

            try:
                line_number = int(raw_line_number)
            except ValueError:
                continue

            line_hits = line_hits_by_file.setdefault(source, {})
            line_hits[line_number] = line_hits.get(line_number, False) or count > 0

    return {
        path: (100.0 * sum(1 for hit in hits.values() if hit) / len(hits), len(hits))
        for path, hits in line_hits_by_file.items()
        if hits
    }


def parse_summary_coverage():
    summary = summary_file.read_text().splitlines()
    cur = None
    parsed = {}
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
                if is_production_cpp(cur):
                    previous = parsed.get(cur)
                    if previous is None or pct > previous[0]:
                        parsed[cur] = (pct, lines)
            cur = None
    return parsed


gcov_dir = coverage_dir / 'gcov-lines'
best = aggregate_line_coverage(gcov_dir) if gcov_dir.exists() else parse_summary_coverage()

count = sum(lines for _, lines in best.values())
total = sum(pct * lines for pct, lines in best.values())
if count:
    overall = total / count
    print(f"Overall project coverage: {overall:.2f}% ({count} lines)")

    print("\nLowest covered production files:")
    for path, (pct, lines) in sorted(best.items(), key=lambda item: item[1][0])[:10]:
        rel = Path(path).relative_to(project_root)
        print(f"  {pct:6.2f}%  {lines:4d} lines  {rel}")

    undercovered_files = []
    if minimum_percent is not None:
        undercovered_files = [
            (path, pct, lines)
            for path, (pct, lines) in sorted(best.items(), key=lambda item: item[1][0])
            if pct < minimum_percent
        ]

    if minimum_percent is not None and overall < minimum_percent:
        print(
            f"\nCoverage gate failed: {overall:.2f}% is below required {minimum_percent:.2f}%",
            file=sys.stderr,
        )
        sys.exit(2)

    if undercovered_files:
        print(
            f"\nPer-file coverage gate failed: {len(undercovered_files)} production file(s) below "
            f"{minimum_percent:.2f}%",
            file=sys.stderr,
        )
        for path, pct, lines in undercovered_files:
            rel = Path(path).relative_to(project_root)
            print(f"  {pct:6.2f}%  {lines:4d} lines  {rel}", file=sys.stderr)
        sys.exit(2)

    if minimum_percent is not None:
        print(f"\nCoverage gate passed: overall {overall:.2f}% and every production file >= {minimum_percent:.2f}%")
else:
    print("Overall project coverage: unavailable")
    if minimum_percent is not None:
        sys.exit(2)
