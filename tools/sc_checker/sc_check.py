#!/usr/bin/env python3
"""sc_check.py — orchestrates all checker modules and prints pass/fail report."""

from __future__ import annotations

import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, List, Dict, Any

import check_tier1_regex
import check_tier2_ast
import check_tier3_callgraph

Finding = Dict[str, Any]
CheckFn = Callable[[Path], List[Finding]]


@dataclass
class CheckEntry:
    name: str
    fn: CheckFn


CHECKS = [
    CheckEntry("tier1_regex", check_tier1_regex.run),
    CheckEntry("tier2_ast", check_tier2_ast.run),
    CheckEntry("tier3_callgraph", check_tier3_callgraph.run),
]


def run_all_checks(target: Path) -> tuple[list[dict], list[dict]]:
    """Return (check_statuses, findings)."""
    statuses: list[dict] = []
    findings: list[dict] = []

    for check in CHECKS:
        try:
            result = check.fn(target)
            if not isinstance(result, list):
                statuses.append({"name": check.name, "passed": False, "error": "check did not return a list"})
                continue
            statuses.append({"name": check.name, "passed": True, "findings": len(result)})
            findings.extend(result)
        except Exception as exc:  # pragma: no cover - defensive path
            statuses.append({"name": check.name, "passed": False, "error": str(exc)})

    return statuses, findings


def main(argv: list[str] | None = None) -> int:
    argv = argv if argv is not None else sys.argv[1:]
    target = Path(argv[0]) if argv else Path(".")
    if not target.is_dir():
        print(f"Error: '{target}' is not a directory.", file=sys.stderr)
        return 1

    statuses, findings = run_all_checks(target)

    print(f"sc_check — scanning {target.resolve()}")
    print("=" * 60)
    print("Checks:")
    for st in statuses:
        if st["passed"]:
            print(f"  [PASS] {st['name']} (findings: {st['findings']})")
        else:
            print(f"  [FAIL] {st['name']} ({st['error']})")

    print("-" * 60)
    if findings:
        print("Findings:")
        for f in findings:
            sev = f.get("severity", "warning")
            print(f"  [{sev}] [{f.get('check','?')}] {f.get('file','?')}:{f.get('line','?')} — {f.get('message','')}")
    else:
        print("Findings: none")

    errors = [f for f in findings if f.get("severity") == "error"]
    failed_checks = [st for st in statuses if not st["passed"]]
    return 1 if (errors or failed_checks) else 0


if __name__ == "__main__":
    raise SystemExit(main())
