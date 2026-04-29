# sc_checker

## Usage

Run all checks and get a PASS/FAIL report:

```bash
python sc_check.py [directory]
```

## Structure

- `sc_check.py` — orchestrator, runs all checks and prints report
- `check_tier1_regex.py` — tier1 check module
- `check_tier2_ast.py` — tier2 check module
- `check_tier3_callgraph.py` — tier3 check module
- `tests/test_checks.py` — pytest suite for checker orchestration/modules
