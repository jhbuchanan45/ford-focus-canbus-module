# PC Development Emulation Spec

## Overview

The host build (`CANMOD_TARGET=host`) produces a `canmod-host` binary that runs on x86 Linux using SocketCAN (`vcan0`) as its CAN peripheral. Real captured CAN logs can be replayed through the binary for offline development and regression testing without the vehicle.

## Requirements

### Requirement: Real CAN captures can be replayed for regression testing
Captured CAN log files (`.log` format from `candump` or `.asc` from other tools) SHALL be replayable against the host build using `canplayer`. This allows full signal decode regression testing without the vehicle.

`tools/replay.sh` SHALL additionally support a `--golden <file>` flag. When provided, the script captures `canmod-host` JSON output, strips `"ts"` fields, and diffs against the golden file. The script exits non-zero and prints a unified diff if the output does not match.

#### Scenario: Log file replayed
- **WHEN** `canplayer -I capture.log vcan0` is run while `canmod-host` is running
- **THEN** the host binary processes all frames from the log file and produces JSON output equivalent to what would have been produced on real hardware

#### Scenario: Golden-file assertion passes on matching output
- **WHEN** `tools/replay.sh forscan_baseline.log vcan0 --golden forscan_baseline_golden.ndjson` is run and the JSON output (after timestamp stripping) matches the golden file
- **THEN** the script exits with code 0

#### Scenario: Golden-file assertion fails on signal regression
- **WHEN** a decode function change causes a signal value to differ from the golden baseline
- **THEN** the script prints a unified diff of the differing lines and exits with code 1
