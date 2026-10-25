#!/usr/bin/env python3
###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2026 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

"""
Generate a Markdown report of compiler / clang-tidy warnings from build logs.

Warnings are deduplicated (the same header warning is printed once per
translation unit), restricted to project files (dependencies, system headers
and generated files are skipped), and split into "in changed files" and "total".

Usage:
  warnings_report.py --workspace DIR [--changed-files FILE] [--build-dir NAME]
      --log "Title=path/to/build.log" [--log ...] > report.md
"""

import argparse
import collections
import posixpath
import re
import sys

# file:line:col: warning: message [flag-or-check]
WARNING_RE = re.compile(
    r"^(?P<file>[^\s:][^:]*):(?P<line>\d+):(?P<col>\d+): warning: (?P<msg>.*?)(?: \[(?P<flag>[^\]]+)\])?$")

# Paths (relative to the workspace) whose warnings are not ours.
EXCLUDED_PREFIXES = ("dependencies/",)

# Maximum number of warnings listed per section, to keep the comment within GitHub's size limit.
MAX_LISTED = 150

MARKER = "<!-- gsmartcontrol-warnings-report -->"


Warning = collections.namedtuple("Warning", "file line col msg flag")


def parse_log(path, workspace, build_dirs):
	"""Return a sorted list of unique project warnings found in the log."""
	found = set()
	try:
		with open(path, encoding="utf-8", errors="replace") as f:
			lines = f.readlines()
	except FileNotFoundError:
		return None

	for raw in lines:
		m = WARNING_RE.match(raw.rstrip("\r\n"))
		if not m:
			continue
		file = posixpath.normpath(m.group("file"))
		if posixpath.isabs(file):
			file = posixpath.relpath(file, workspace)
		if file.startswith("../") or file.startswith(EXCLUDED_PREFIXES):
			continue  # system headers, dependencies
		if any(file == d or file.startswith(d + "/") for d in build_dirs):
			continue  # generated files
		found.add(Warning(file, int(m.group("line")), int(m.group("col")),
				m.group("msg").strip(), (m.group("flag") or "").replace(",-warnings-as-errors", "")))

	return sorted(found)


def format_list(warnings):
	out = []
	for w in warnings[:MAX_LISTED]:
		flag = f" `[{w.flag}]`" if w.flag else ""
		msg = w.msg.replace("|", "\\|").replace("`", "'")
		out.append(f"- `{w.file}:{w.line}:{w.col}`: {msg}{flag}")
	if len(warnings) > MAX_LISTED:
		out.append(f"- ... and {len(warnings) - MAX_LISTED} more")
	return "\n".join(out)


def format_top_flags(warnings, count=15):
	counter = collections.Counter(w.flag or "(no flag)" for w in warnings)
	rows = ["| Warning | Count |", "|---|---:|"]
	rows += [f"| `{flag}` | {n} |" for flag, n in counter.most_common(count)]
	return "\n".join(rows)


def main():
	parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
	parser.add_argument("--workspace", required=True, help="Source root; warnings are reported relative to it")
	parser.add_argument("--build-dir", action="append", default=[], help="Build directory (relative), skipped")
	parser.add_argument("--changed-files", help="File with one changed path (relative) per line")
	parser.add_argument("--log", action="append", required=True, help="Title=path of a build log")
	args = parser.parse_args()

	workspace = posixpath.abspath(args.workspace)
	changed = None
	if args.changed_files:
		with open(args.changed_files, encoding="utf-8") as f:
			changed = {line.strip() for line in f if line.strip()}

	reports = []
	for spec in args.log:
		title, _, path = spec.partition("=")
		reports.append((title, parse_log(path, workspace, args.build_dir)))

	out = [MARKER, "## Warnings report", ""]

	header = "| | " + " | ".join(title for title, _ in reports) + " |"
	out += [header, "|---" + "|---:" * len(reports) + "|"]
	if changed is not None:
		cells = []
		for _, warnings in reports:
			cells.append("n/a" if warnings is None else str(sum(1 for w in warnings if w.file in changed)))
		out.append("| In changed files | " + " | ".join(cells) + " |")
	cells = ["build log missing" if w is None else str(len(w)) for _, w in reports]
	out.append("| Total (deduplicated) | " + " | ".join(cells) + " |")
	out.append("")

	for title, warnings in reports:
		if not warnings:
			continue
		if changed is not None:
			in_changed = [w for w in warnings if w.file in changed]
			if in_changed:
				out += [f"<details open><summary><b>{title}</b>: {len(in_changed)} in changed files</summary>", "",
						format_list(in_changed), "", "</details>", ""]
		out += [f"<details><summary><b>{title}</b>: all {len(warnings)} warnings by type</summary>", "",
				format_top_flags(warnings), "", "</details>", ""]

	out.append("<sub>Warnings in `dependencies/`, system headers and generated files are excluded. "
			"Updated automatically on each push.</sub>")
	print("\n".join(out))
	return 0


if __name__ == "__main__":
	sys.exit(main())
