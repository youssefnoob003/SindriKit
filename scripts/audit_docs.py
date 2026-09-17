#!/usr/bin/env python3
"""Documentation integrity audit for SindriKit.

Checks, across the user-facing Markdown files:

1. Relative link targets exist on disk.
2. Same-file ``](#anchor)`` links resolve to a real heading slug.
3. Code fences are balanced.
4. Every ``snd_*`` identifier mentioned in prose exists somewhere in the
   code tree (so docs cannot drift from the headers).

Exit code is non-zero when anything fails. Stdlib only; run from the repo
root (``python scripts/audit_docs.py``).
"""

import os
import re
import sys

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

SCAN_ROOT_MD = ["README.md", "SECURITY.md", "CONTRIBUTING.md"]
CODE_DIRS = ["include", "src", "pocs", "tests"]
CODE_EXTS = (".c", ".h", ".asm")

# Identifiers that are intentionally illustrative or documented as absent.
IGNORE_IDENTIFIERS = {
    "snd_mod_sys",        # explicitly documented as "no such backend"
    "snd_ldr_xyz_ctx_t",  # placeholder in a naming example
    "snd_crtless_poc_entry",
}

LINK_RE = re.compile(r"\[[^\]]*\]\(([^)]+)\)")
IDENT_RE = re.compile(r"\bsnd_[a-z0-9_]+\b")
HEADING_RE = re.compile(r"^(#{1,6})\s+(.*)$", re.MULTILINE)
ANCHOR_RE = re.compile(r"\]\(#([^)]+)\)")


def iter_docs():
    for name in SCAN_ROOT_MD:
        path = os.path.join(REPO_ROOT, name)
        if os.path.exists(path):
            yield path
    docs_dir = os.path.join(REPO_ROOT, "docs")
    for dirpath, _, files in os.walk(docs_dir):
        for f in files:
            if f.endswith(".md"):
                yield os.path.join(dirpath, f)


def scan_code_identifiers():
    found = set()
    roots = [os.path.join(REPO_ROOT, rel) for rel in CODE_DIRS]
    roots.append(REPO_ROOT)
    for base in roots:
        for dirpath, _, files in os.walk(base):
            if f"{os.sep}.git" in dirpath:
                continue
            for f in files:
                if not (f.endswith(CODE_EXTS) or f == "CMakeLists.txt"):
                    continue
                try:
                    text = open(os.path.join(dirpath, f), encoding="utf-8", errors="ignore").read()
                except OSError:
                    continue
                found.update(IDENT_RE.findall(text))
    return found


def slugify(heading):
    s = heading.strip().lower().replace("`", "")
    s = re.sub(r"[^\w\s\-]", "", s)
    return re.sub(r"\s+", "-", s.strip())


def main():
    code_ids = scan_code_identifiers()
    failures = []

    for path in sorted(iter_docs()):
        rel = os.path.relpath(path, REPO_ROOT)
        text = open(path, encoding="utf-8").read()

        if text.count("```") % 2 != 0:
            failures.append(f"{rel}: unbalanced code fences")

        headings = {slugify(m.group(2)) for m in HEADING_RE.finditer(text)}

        for m in LINK_RE.finditer(text):
            target = m.group(1).strip()
            if not target or "://" in target or target.startswith("mailto:"):
                continue
            file_part = target.split("#", 1)[0]
            if not file_part:
                continue  # same-file anchor, handled below
            resolved = os.path.normpath(os.path.join(os.path.dirname(path), file_part))
            if not os.path.exists(resolved):
                failures.append(f"{rel}: broken link -> {target}")

        for m in ANCHOR_RE.finditer(text):
            anchor = m.group(1)
            if anchor not in headings:
                failures.append(f"{rel}: unresolved anchor -> #{anchor}")

        # identifier coverage (strip link targets so lowercased TOC anchors are ignored)
        prose = re.sub(r"\]\([^)]*\)", "", text)
        for ident in set(IDENT_RE.findall(prose)):
            if ident in code_ids or ident in IGNORE_IDENTIFIERS or ident.endswith("_"):
                continue
            failures.append(f"{rel}: unknown identifier -> {ident}")

    if failures:
        print(f"audit_docs: {len(failures)} problem(s)")
        for f in failures:
            print("  " + f)
        return 1

    print("audit_docs: OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
