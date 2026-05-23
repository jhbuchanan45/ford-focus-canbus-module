## ADDED Requirements

### Requirement: Golden-file regression test activates automatically when capture files exist
A regression test (`tests/test_regression.c`) SHALL replay `tools/captures/forscan_baseline.log` through `canmod-host` and compare the JSON output line-by-line against `tools/captures/forscan_baseline_golden.ndjson`. The test SHALL be skipped (not failed) if either file is absent.

#### Scenario: Regression test skips when no capture file exists
- **WHEN** `ctest` is run and `tools/captures/forscan_baseline.log` does not exist
- **THEN** the regression test is reported as `Not Run` and the overall suite still passes

#### Scenario: Regression test passes when output matches golden file
- **WHEN** the replay of `forscan_baseline.log` produces JSON output identical to `forscan_baseline_golden.ndjson` (after stripping `"ts"` fields)
- **THEN** the regression test passes

#### Scenario: Regression test fails when a signal decode changes
- **WHEN** a decode function is modified such that a signal value changes for a known frame
- **THEN** the regression test reports a diff between actual and golden output and fails

---

### Requirement: Timestamps are stripped before golden-file comparison
The regression comparison SHALL strip the `"ts":<number>` field from each JSON line before diffing, since timestamps differ between replay runs.

#### Scenario: Two replays of the same log produce matching output
- **WHEN** `forscan_baseline.log` is replayed twice at different wall-clock times
- **THEN** both outputs match the golden file after timestamp stripping

---

### Requirement: tools/replay.sh supports a --golden assertion mode
`tools/replay.sh` SHALL accept an optional `--golden <file>` argument. When provided, after the replay completes it SHALL diff the captured JSON output (with timestamps stripped) against the golden file and exit non-zero if they differ.

#### Scenario: replay.sh --golden exits 0 on match
- **WHEN** `tools/replay.sh forscan_baseline.log vcan0 --golden forscan_baseline_golden.ndjson` is run and output matches
- **THEN** the script exits with code 0

#### Scenario: replay.sh --golden exits non-zero on mismatch
- **WHEN** the JSON output differs from the golden file
- **THEN** the script prints a unified diff and exits with code 1
