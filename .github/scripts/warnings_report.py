#!/usr/bin/env python3
###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2026 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

"""Generate a Markdown report of compiler / clang-tidy warnings from CI build logs."""

# Each build log directory (as saved by .github/actions/save-build-log) contains:
#   build.log      - Build output.
#   title.txt      - Platform / compiler name shown in the report.
#   category.txt   - "compiler" or "clang-tidy".
#   workspace.txt  - Source root path, as printed by the compiler on that platform.
#
# Warnings are deduplicated (the same header warning is printed once per
# translation unit, and the same warning may be reported by several platforms),
# restricted to project files (dependencies, system headers and generated files
# are skipped), and split into "in files changed by the PR" and "total".
#
# Usage:
#   warnings_report.py --logs-dir DIR [--changed-files FILE] [--comment FILE] [--full FILE] [--text FILE]
#       [--repo OWNER/NAME --sha SHA] [--run-url URL]

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

# Maximum number of warnings listed for changed files.
MAX_LISTED = 200

# GitHub's comment limit is 65536 characters; a job summary can be up to 1 MiB.
MAX_REPORT_SIZE = 60000
MAX_FULL_REPORT_SIZE = 1000000

MARKER = "<!-- gsmartcontrol-warnings-report -->"

COMPILER = "compiler"
CLANG_TIDY = "clang-tidy"


CompilerWarning = collections.namedtuple("CompilerWarning", "file line col msg flag")


class BuildLog:
	"""Warnings parsed from one build log."""

	def __init__(self, title, category, warnings):
		"""Store the build title, category and warnings (None if the log is missing)."""
		self.title = title
		self.category = category
		self.warnings = warnings  # set of CompilerWarning, or None if the log is missing


def normalize_path(path):
	path = path.replace("\\", "/")
	if re.match(r"^[A-Za-z]:/", path):
		path = path[0].lower() + path[1:]
	return posixpath.normpath(path)


def to_repo_path(file, workspace):
	"""Return the path relative to the repository root, or None if outside it."""
	file = normalize_path(file)
	if workspace and (file == workspace or file.startswith(workspace + "/")):
		return file[len(workspace) + 1:]
	if file.startswith("/") or re.match(r"^[a-z]:/", file) or file.startswith("../"):
		return None  # system headers, etc.
	return file


def read_text(path, default=""):
	return path.read_text(encoding="utf-8").strip() if path.exists() else default


def parse_log(log_dir):
	title = read_text(log_dir / "title.txt") or log_dir.name.removeprefix("build-log-")
	category = read_text(log_dir / "category.txt", COMPILER)
	workspace = normalize_path(read_text(log_dir / "workspace.txt"))

	log_file = log_dir / "build.log"
	if not log_file.exists():
		return BuildLog(title, category, None)

	found = set()
	for raw in log_file.read_text(encoding="utf-8", errors="replace").splitlines():
		m = WARNING_RE.match(raw.rstrip())
		if not m:
			continue
		file = to_repo_path(m.group("file"), workspace)
		if file is None or file.startswith(EXCLUDED_PREFIXES):
			continue
		flag = (m.group("flag") or "").replace(",-warnings-as-errors", "")
		if category == CLANG_TIDY and (flag.startswith("-W") or flag.startswith("clang-diagnostic-")):
			continue  # compiler diagnostics; these are reported by the compiler builds
		found.add(CompilerWarning(file, int(m.group("line")), int(m.group("col")), m.group("msg").strip(), flag))
	return BuildLog(title, category, found)


def escape(text):
	return text.replace("|", "\\|").replace("`", "'").replace("<", "&lt;").replace(">", "&gt;")


def shorten(text, limit=300):
	return text if len(text) <= limit else text[:limit - 1] + "…"


def plural(n, word):
	return f"{n} {word}" if n == 1 else f"{n} {word}s"


class Report:
	"""Markdown warnings report for a set of build logs."""

	def __init__(self, logs, changed, repo, sha, run_url):
		"""Index the warnings of all logs by category."""
		self.logs = logs
		self.changed = changed
		self.repo = repo
		self.sha = sha
		self.run_url = run_url

		# Unique warnings per category, with the build titles that reported each.
		self.reported_by = {COMPILER: collections.defaultdict(list), CLANG_TIDY: collections.defaultdict(list)}
		for log in logs:
			for w in log.warnings or ():
				self.reported_by[log.category][w].append(log.title)

	def logs_of(self, category):
		return [log for log in self.logs if log.category == category]

	def unique(self, category):
		return sorted(self.reported_by[category])

	def in_changed(self, warnings):
		return [w for w in warnings if w.file in self.changed] if self.changed is not None else []

	def link(self, w):
		text = f"{w.line}:{w.col}"
		if self.repo and self.sha:
			return f"[{text}](https://github.com/{self.repo}/blob/{self.sha}/{w.file}#L{w.line})"
		return text

	def reporters(self, category, w):
		titles = self.reported_by[category][w]
		available = [log.title for log in self.logs_of(category) if log.warnings is not None]
		if category == COMPILER and len(available) > 1 and len(titles) == len(available):
			return "all compilers"
		return ", ".join(titles)

	def headline(self):
		lines = []
		compiler = self.unique(COMPILER)
		tidy = self.unique(CLANG_TIDY)
		if self.changed is not None:
			c, t = len(self.in_changed(compiler)), len(self.in_changed(tidy))
			if c == 0 and t == 0:
				lines.append("### ✅ No warnings in files changed by this PR")
			else:
				parts = []
				if c:
					parts.append(plural(c, 'compiler warning'))
				if t:
					parts.append(plural(t, 'clang-tidy warning'))
				lines.append(f"### ⚠️ {' and '.join(parts)} in files changed by this PR")
		else:
			lines.append("### Warnings report")
		lines.append("")
		lines.append(f"Whole project: {plural(len(compiler), 'compiler warning')}, "
				f"{plural(len(tidy), 'clang-tidy warning')} (unique).")

		missing = [log.title for log in self.logs if log.warnings is None]
		if missing:
			lines += ["", f"> [!WARNING]\n> No build log from: {escape(', '.join(missing))} "
					"(the build failed early or was cancelled), so their warnings are not included."]
		return lines

	def platform_table(self, category, total_label):
		logs = self.logs_of(category)
		if not logs:
			return []
		with_changed = self.changed is not None
		header = ["| Build | In changed files | Total |", "|---|---:|---:|"] if with_changed \
			else ["| Build | Total |", "|---|---:|"]
		rows = []
		for log in logs:
			if log.warnings is None:
				cells = ["–", "❌ no log"] if with_changed else ["❌ no log"]
			else:
				cells = ([str(len(self.in_changed(log.warnings)))] if with_changed else []) + [str(len(log.warnings))]
			rows.append(f"| {escape(log.title)} | " + " | ".join(cells) + " |")
		if len(logs) > 1:
			unique = self.unique(category)
			cells = ([f"**{len(self.in_changed(unique))}**"] if with_changed else []) + [f"**{len(unique)}**"]
			rows.append(f"| **{total_label}** | " + " | ".join(cells) + " |")
		return header + rows + [""]

	def all_items(self, changed_only):
		items = [(COMPILER, w) for w in self.unique(COMPILER)] + [(CLANG_TIDY, w) for w in self.unique(CLANG_TIDY)]
		if changed_only:
			items = [(category, w) for category, w in items if self.changed is not None and w.file in self.changed]
		return items

	def warnings_section(self, heading, items, max_listed, open_files):
		"""List warnings grouped by file. max_listed=None lists all of them."""
		if not items:
			return []
		by_file = collections.defaultdict(list)
		for category, w in items:
			by_file[w.file].append((category, w))

		out = [heading, ""]
		listed = 0
		for file in sorted(by_file):
			if max_listed is not None and listed >= max_listed:
				out += [f"_Only the first {listed} of {len(items)} warnings are listed here; "
						"see the full report for all of them._", ""]
				break
			entries = sorted(by_file[file], key=lambda e: (e[1].line, e[1].col, e[0]))
			out += [f"<details{' open' if open_files else ''}><summary><code>{escape(file)}</code> "
					f"({len(entries)})</summary>", "",
					"| Line | Warning | Type | Reported by |", "|---|---|---|---|"]
			for category, w in entries:
				if max_listed is not None and listed >= max_listed:
					break
				flag = f"`{w.flag}`" if w.flag else ""
				out.append(f"| {self.link(w)} | {escape(shorten(w.msg))} | {flag} | {escape(self.reporters(category, w))} |")
				listed += 1
			out += ["", "</details>", ""]
		return out

	def breakdown_section(self):
		compiler = self.unique(COMPILER)
		tidy = self.unique(CLANG_TIDY)
		if not compiler and not tidy:
			return []
		out = []

		by_type = collections.Counter()
		for category, warnings in ((COMPILER, compiler), (CLANG_TIDY, tidy)):
			for w in warnings:
				by_type[(category, w.flag or "(no flag)")] += 1
		out += ["<details><summary><b>Whole project: warnings by type</b></summary>", "",
				"| Warning | Source | Count |", "|---|---|---:|"]
		out += [f"| `{flag}` | {category} | {n} |" for (category, flag), n in by_type.most_common(30)]
		out += ["", "</details>", ""]

		by_file = collections.defaultdict(lambda: [0, 0])
		for w in compiler:
			by_file[w.file][0] += 1
		for w in tidy:
			by_file[w.file][1] += 1
		top = sorted(by_file.items(), key=lambda kv: (-sum(kv[1]), kv[0]))[:20]
		out += ["<details><summary><b>Whole project: files with most warnings</b></summary>", "",
				"| File | Compiler | clang-tidy |", "|---|---:|---:|"]
		out += [f"| `{escape(file)}` | {c} | {t} |" for file, (c, t) in top]
		out += ["", "</details>", ""]
		return out

	def footer(self, full):
		parts = []
		if self.sha:
			parts.append(f"Built from commit {self.sha[:10]}")
		if self.run_url and not full:
			parts.append(f"[Full report and build logs]({self.run_url})")
		parts.append("Deduplicated across translation units and builds. "
				"Excludes `dependencies/`, system headers and generated files.")
		return ["<sub>" + " · ".join(parts) + "</sub>"]

	def full_report_link(self):
		total = len(self.all_items(changed_only=False))
		if not self.run_url or not total:
			return []
		return [f"🔎 **[Inspect all {total} warnings]({self.run_url})**: the full report is in the workflow run's "
				"summary; the `warnings-report` artifact has it as Markdown and as `warnings.txt` "
				"(`file:line:col: warning: ...`, for IDEs).", ""]

	def render_with(self, full, max_listed, details=True):
		out = [] if full else [MARKER]
		out += self.headline() + [""]
		if not full:
			out += self.full_report_link()
		if details:
			out += self.warnings_section("#### Warnings in changed files", self.all_items(changed_only=True),
					max_listed, open_files=True)
		compiler_table = self.platform_table(COMPILER, "All compilers (unique)")
		if compiler_table:
			out += ["#### Compiler warnings", ""] + compiler_table
		tidy_table = self.platform_table(CLANG_TIDY, "All (unique)")
		if tidy_table:
			out += ["#### clang-tidy", ""] + tidy_table
		if details:
			if full:
				out += self.warnings_section("#### All warnings", self.all_items(changed_only=False),
						max_listed, open_files=False)
			out += self.breakdown_section()
		else:
			out += ["_The detailed report is too large to show here; "
					"see the `warnings-report` artifact of the workflow run._", ""]
		out += self.footer(full)
		return "\n".join(out)

	def render(self, full):
		"""Render the PR comment (or the full report), listing fewer warnings until it fits."""
		max_size, limits = (MAX_FULL_REPORT_SIZE, (None, 5000, 2000, 1000, 500)) if full \
			else (MAX_REPORT_SIZE, (MAX_LISTED, 100, 50, 20))
		for max_listed in limits:
			text = self.render_with(full, max_listed)
			if len(text) <= max_size:
				return text
		text = self.render_with(full, 0, details=False)
		if len(text) > max_size:  # Only with an absurd number of builds
			text = text[:text.rfind("\n", 0, max_size)] + "\n\n_The report was truncated._"
		return text

	def render_text(self):
		"""All unique warnings in compiler format, for IDEs and grep."""
		lines = []
		for w in sorted({w for _, w in self.all_items(changed_only=False)}):
			flag = f" [{w.flag}]" if w.flag else ""
			lines.append(f"{w.file}:{w.line}:{w.col}: warning: {w.msg}{flag}")
		return "\n".join(lines) + ("\n" if lines else "")


def write_output(path, text):
	if path == "-":
		sys.stdout.write(text + "\n")
	else:
		pathlib.Path(path).write_text(text + ("" if text.endswith("\n") else "\n"), encoding="utf-8")


def main():
	parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
	parser.add_argument("--logs-dir", required=True, help="Directory with one subdirectory per build log")
	parser.add_argument("--changed-files", help="File with one changed path (relative) per line")
	parser.add_argument("--repo", help="owner/name, for links to source lines")
	parser.add_argument("--sha", help="SHA of the commit that was built, for links to source lines")
	parser.add_argument("--run-url", help="URL of the workflow run")
	parser.add_argument("--comment", default="-", help="Output file for the PR comment (default: stdout)")
	parser.add_argument("--full", help="Output file for the full report (all warnings)")
	parser.add_argument("--text", help="Output file for all warnings in compiler format")
	args = parser.parse_args()

	changed = None
	if args.changed_files:
		with open(args.changed_files, encoding="utf-8") as f:
			changed = {line.strip() for line in f if line.strip()}

	logs_dir = pathlib.Path(args.logs_dir)
	logs = [parse_log(d) for d in logs_dir.iterdir() if d.is_dir()] if logs_dir.is_dir() else []
	logs.sort(key=lambda log: log.title.lower())

	sys.stdout.reconfigure(encoding="utf-8")
	report = Report(logs, changed, args.repo, args.sha, args.run_url)
	write_output(args.comment, report.render(full=False))
	if args.full:
		write_output(args.full, report.render(full=True))
	if args.text:
		write_output(args.text, report.render_text())
	return 0


if __name__ == "__main__":
	sys.exit(main())
