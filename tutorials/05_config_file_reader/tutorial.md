# Tutorial: Config File Reader (RAII + strings)

## Goal

Build a small key/value config reader that uses `AUTO_FILE` and returns early safely on malformed lines.

## Why this belongs in tutorials

It has branching parse paths and multiple cleanup points, so it is better as a guided walkthrough than a tiny example.

## Full solution

See `solution.c`.
