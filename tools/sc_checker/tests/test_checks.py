"""Tests for sc_check orchestrator and per-check modules."""

from pathlib import Path

import check_tier1_regex
import check_tier2_ast
import check_tier3_callgraph
import sc_check


def test_individual_checks_return_list(tmp_path: Path):
    assert isinstance(check_tier1_regex.run(tmp_path), list)
    assert isinstance(check_tier2_ast.run(tmp_path), list)
    assert isinstance(check_tier3_callgraph.run(tmp_path), list)


def test_run_all_checks_reports_pass(tmp_path: Path):
    statuses, findings = sc_check.run_all_checks(tmp_path)
    assert len(statuses) == 3
    assert all(s["passed"] for s in statuses)
    assert isinstance(findings, list)


def test_main_returns_error_for_missing_directory(tmp_path: Path):
    missing = tmp_path / "does_not_exist"
    rc = sc_check.main([str(missing)])
    assert rc == 1


def test_main_success_for_valid_directory(tmp_path: Path):
    rc = sc_check.main([str(tmp_path)])
    assert rc == 0
