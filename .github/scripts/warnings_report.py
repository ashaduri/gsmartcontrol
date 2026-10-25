#!/usr/bin/env python3
###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2026 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

"""
Generate a Markdown report of compiler / clang-tidy warnings from CI build logs.

Each build log directory (as saved by .github/actions/save-build-log) contains:
  build.log      - Build output.
  title.txt      - Platform / compiler name shown in the report.
  workspace.txt  - Source root path, as printed by the compiler on that platform.

Warnings are deduplicated (the same header warning is printed once per
translation unit, and the same warning may be reported by several platforms),
restricted to project files (dependencies, system headers and generated files
are skipped), and split into "in changed files" and "total".

Usage:
  warnings_report.py --logs-dir DIR [--changed-files FILE] > report.md
"""

import argparse
import collections
import pathlib
import posixpath
import re
import sys

# file:line:col: warning: message [flag-or-check]
# The file may start with a Windows drive letter (D:/a/...).
WARNING_RE = re.compile(
	r"^(?P<file>(?:[A-Za-z]:)?[^\s:][^:]*):(?P<line>\d+):(?P<col>\d+): warning: (?P<msg>.*?)(?: \[(?P<flag>[^\]]+)\])?$")

# Repository paths whose warnings are not ours.
EXCLUDED_PREFIXES = ("dependencies/", "build/")

# Maximum number of warnings listed, to keep the comment within GitHub's size limit.
MAX_LISTED = 150

MARKER = "<!-- gsmartcontrol-warnings-report -->"


Warning = collections.namedtuple("Warning", "file line col msg flag")


def normalize_path(path):
	path = path.replace("\\", "/")
	if re.match(r"^[A-Za-z]:/", path):
		path = path[0].lower() + path[1:]
	elif re.match(r"^/[A-Za-z]/", path):  # msys-style /d/a/... -> d:/a/...
		path = path[1].lower() + ":" + path[2:]
	return posixpath.normpath(path)


def to_repo_path(file, workspace):
	"""Return the path relative to the repository root, or None if outside it."""
	file = normalize_path(file)
	if workspace and (file == workspace or file.startswith(workspace + "/")):
		return file[len(workspace) + 1:]
	if file.startswith("/") or re.match(r"^[a-z]:/", file) or file.startswith("../"):
		return None  # system headers, etc.
	return file


def parse_log(log_dir):
	"""Return (title, set of warnings), or (title, None) if the log is missing."""
	title = log_dir.name.removeprefix("build-log-")
	title_file = log_dir / "title.txt"
	if title_file.exists():
		title = title_file.read_text(encoding="utf-8").strip() or title
	workspace_file = log_dir / "workspace.txt"
	workspace = normalize_path(workspace_file.read_text(encoding="utf-8").strip()) if workspace_file.exists() else ""

	log_file = log_dir / "build.log"
	if not log_file.exists():
		return title, None

	found = set()
	for raw in log_file.read_text(encoding="utf-8", errors="replace").splitlines():
		m = WARNING_RE.match(raw.rstrip())
		if not m:
			continue
		file = to_repo_path(m.group("file"), workspace)
		if file is None or file.startswith(EXCLUDED_PREFIXES):
			continue
		flag = (m.group("flag") or "").replace(",-warnings-as-errors", "")
		found.add(Warning(file, int(m.group("line")), int(m.group("col")), m.group("msg").strip(), flag))
	return title, found


def escape(text):
	return text.replace("|", "\\|").replace("`", "'").replace("<", "&lt;")


def main():
	parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
	parser.add_argument("--logs-dir", required=True, help="Directory with one subdirectory per build log")
	parser.add_argument("--changed-files", help="File with one changed path (relative) per line")
	args = parser.parse_args()

	changed = None
	if args.changed_files:
		with open(args.changed_files, encoding="utf-8") as f:
			changed = {line.strip() for line in f if line.strip()}

	reports = [parse_log(d) for d in pathlib.Path(args.logs_dir).iterdir() if d.is_dir()]
	reports.sort(key=lambda r: r[0].lower())

	# Unique warnings across all platforms, with the platforms that reported each.
	platforms_by_warning = collections.defaultdict(list)
	for title, warnings in reports:
		for w in warnings or ():
			platforms_by_warning[w].append(title)
	all_warnings = sorted(platforms_by_warning)

	def in_changed(warnings):
		return [w for w in warnings if w.file in changed]

	out = [MARKER, "## Warnings report", ""]

	if changed is not None:
		out += ["| Platform | In changed files | Total |", "|---|---:|---:|"]
	else:
		out += ["| Platform | Total |", "|---|---:|"]
	for title, warnings in reports:
		if warnings is None:
			cells = ["n/a", "build log missing"] if changed is not None else ["build log missing"]
		elif changed is not None:
			cells = [str(len(in_changed(warnings))), str(len(warnings))]
		else:
			cells = [str(len(warnings))]
		out.append(f"| {escape(title)} | " + " | ".join(cells) + " |")
	if changed is not None:
		out.append(f"| **All platforms (unique)** | **{len(in_changed(all_warnings))}** | **{len(all_warnings)}** |")
	else:
		out.append(f"| **All platforms (unique)** | **{len(all_warnings)}** |")
	out.append("")

	listed = in_changed(all_warnings) if changed is not None else []
	if listed:
		out += [f"<details open><summary><b>Warnings in changed files ({len(listed)})</b></summary>", ""]
		for w in listed[:MAX_LISTED]:
			flag = f" `[{w.flag}]`" if w.flag else ""
			out.append(f"- `{w.file}:{w.line}:{w.col}`: {escape(w.msg)}{flag}"
					f" <sub>({escape(', '.join(platforms_by_warning[w]))})</sub>")
		if len(listed) > MAX_LISTED:
			out.append(f"- ... and {len(listed) - MAX_LISTED} more")
		out += ["", "</details>", ""]

	if all_warnings:
		counter = collections.Counter(w.flag or "(no flag)" for w in all_warnings)
		out += [f"<details><summary><b>All warnings by type ({len(all_warnings)})</b></summary>", "",
				"| Warning | Count |", "|---|---:|"]
		out += [f"| `{flag}` | {n} |" for flag, n in counter.most_common(25)]
		out += ["", "</details>", ""]

	out.append("<sub>Deduplicated across translation units and platforms. Warnings in `dependencies/`, "
			"system headers and generated files are excluded. Updated automatically on each push.</sub>")
	print("\n".join(out))
	return 0


if __name__ == "__main__":
	sys.exit(main())
