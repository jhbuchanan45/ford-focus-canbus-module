# Development Guide

This project uses **[OpenSpec](https://github.com/Fission-AI/OpenSpec)** for spec-driven development (SDD). The core idea: agree on *what* to build and *how* before writing any firmware code. This keeps AI-assisted development on track, makes scope explicit, and leaves a permanent record of every decision.

---

## The Principle

```
Explore → Specify → Implement → Archive
```

Every change — adding a new CAN signal, refactoring the HAL, adding a UART output format — starts as a written spec. Code only gets written once the spec is agreed. This matters more in firmware than in typical software: a wrong signal scale or off-by-one in a bit offset means wrong data at runtime, and that's hard to debug on hardware.

---

## Anatomy of a Change

OpenSpec tracks each feature as a **change** — a folder under `openspec/changes/` containing:

```
openspec/changes/add-steering-angle/
├── .openspec.yaml     # Change metadata (managed by CLI)
├── proposal.md        # What & why — scope, goals, non-goals
├── design.md          # How — technical approach, decisions, trade-offs
├── tasks.md           # Implementation checklist (drives /opsx:apply)
└── specs/             # Delta specs — what changes relative to current specs
    └── canbus/
        └── spec.md    # ADDED / MODIFIED / REMOVED signals
```

Delta specs only get merged into `openspec/specs/` when you archive the change. Until then, `openspec/specs/` is untouched — the current source of truth stays clean.

---

## Slash Commands

| Command | What it does |
|---|---|
| `/opsx:propose` | Draft all artifacts for a new change in one step |
| `/opsx:explore` | Think through an idea without committing to a spec |
| `/opsx:apply` | Implement the tasks in a change |
| `/opsx:archive` | Finalise a change, sync delta specs → main specs, move to archive |

---

## Small Features

> Use this for: adding a new CAN signal, a new output field, a small decoder fix, a config change.

A small feature goes straight from proposal to implementation without a separate exploration phase.

### 1. Propose

```
/opsx:propose "add coolant fan relay signal on HS-CAN 0x420"
```

Claude will:
- Create `openspec/changes/add-coolant-fan-relay-signal/`
- Generate **proposal.md** — scope and rationale
- Generate **design.md** — which layer is affected (Decoder only for a new signal, or HAL too if pin output needed), signal bit layout
- Generate **tasks.md** — concrete checkbox list
- Generate **specs/canbus/spec.md** — delta spec showing the new signal as `ADDED`

Review the artifacts. Edit them if anything is wrong — especially the bit position, scale, and offset in the delta spec. These are the ground truth for the implementation.

### 2. Apply

```
/opsx:apply
```

Claude reads your proposal, design, and tasks, then works through the checklist: updating `can_decoder.c`, adding the signal to the relevant struct, updating the UART JSON output, and ticking off each task as it goes. It stops and asks if anything is ambiguous.

### 3. Archive

```
/opsx:archive
```

Claude will:
1. Warn if any tasks are still unchecked
2. Show you what the delta spec changes relative to `openspec/specs/canbus/spec.md`
3. Ask whether to sync (merge the delta into the main spec) — **always sync**
4. Move the change folder to `openspec/changes/archive/YYYY-MM-DD-add-coolant-fan-relay-signal/`

After archive, `openspec/specs/canbus/spec.md` reflects the new signal and the change history is preserved in the archive.

---

## Large Features

> Use this for: adding a new CAN bus (e.g. MS-CAN support), designing the HAL abstraction, adding USB CDC output, introducing a test framework.

Large features benefit from an exploration phase before committing to a spec. They may also span multiple sessions.

### 1. Explore First

```
/opsx:explore "add MS-CAN support alongside HS-CAN"
```

Explore mode is a thinking session — no code is written, no spec is committed. Claude will:
- Map the current architecture against the new requirement
- Surface trade-offs (e.g. single vs dual CAN peripheral, shared vs separate decoder config)
- Draw ASCII diagrams of options
- Ask questions to narrow scope

When something crystallises, Claude will offer to capture it: *"That's a design decision — update design.md?"* You decide what gets written down. Exit explore mode naturally when you're ready to formalise.

### 2. Propose

```
/opsx:propose "add ms-can support"
```

Because you've already explored, the proposal phase is faster — Claude has context from the conversation. The artifacts will be more detailed and have fewer surprises:

- **proposal.md**: bus topology, which vehicle models need MS-CAN, explicit non-goals (no simultaneous dual-bus in v0)
- **design.md**: HAL changes (second CAN peripheral init), decoder config struct, build flags for bus selection
- **tasks.md**: broken into phases — HAL first, decoder second, integration test last
- **specs/firmware/spec.md** + **specs/canbus/spec.md**: delta specs for both firmware architecture changes and new message definitions

### 3. Review the Spec Before Implementing

For large features, take a pass over the artifacts *before* running `/opsx:apply`. Check:

- **proposal.md**: Is the scope right? Are the non-goals explicit?
- **design.md**: Are the layer boundaries correct? Does the HAL change break any existing assumptions?
- **tasks.md**: Is each task small enough to implement in one go? If a task is "implement HAL" with no sub-steps, break it down.
- **delta specs**: Correct signal definitions, byte orders, scales?

Edit the files directly — they're just markdown. Claude will re-read them when you invoke `/opsx:apply`.

### 4. Apply in Sessions

Large features usually span multiple sessions. `/opsx:apply` picks up where it left off:

```
/opsx:apply ms-can-support
```

It reads `tasks.md`, finds the first unchecked task, and continues. Completed tasks stay checked between sessions. If implementation reveals a design issue (e.g. the HAL API needs an extra parameter you didn't anticipate), Claude will pause and suggest updating `design.md` before continuing — the spec and code stay in sync.

### 5. Archive

Same as small features, but pay particular attention to the delta spec sync step — large features are more likely to have touched multiple spec files.

```
/opsx:archive
```

---

## Updating Specs Without a Feature

Sometimes you learn something about the Ford Focus bus (a corrected bit position, a signal you had wrong) without it being tied to a feature. In that case:

1. Propose a corrections change:
   ```
   /opsx:propose "correct wheel speed signal byte order"
   ```
2. The proposal will be minimal — just a fix rationale. The delta spec is the important artifact.
3. Apply is trivial (update decoder constants). Archive syncs the correction into `openspec/specs/`.

This keeps `openspec/specs/canbus/spec.md` accurate and gives you a dated record of when each correction was made.

---

## Directory Reference

```
openspec/
├── config.yaml               # Project context fed to Claude when generating artifacts
├── specs/                    # Source of truth — only updated on archive
│   ├── firmware/spec.md      # Firmware architecture (layers, APIs, conventions)
│   └── canbus/spec.md        # CAN message and signal definitions
└── changes/
    ├── <active-change>/      # In-progress changes
    │   ├── .openspec.yaml
    │   ├── proposal.md
    │   ├── design.md
    │   ├── tasks.md
    │   └── specs/            # Delta specs (merged on archive)
    └── archive/
        └── YYYY-MM-DD-<name>/  # Completed changes
```

---

## Tips

**Keep proposals short.** A proposal that's more than one page is probably two features. Split it. Smaller scope = easier to implement, easier to review, easier to revert.

**Non-goals are as important as goals.** If MS-CAN support explicitly excludes simultaneous dual-bus in v0, write that in the proposal. It stops scope creep mid-implementation.

**Let the tasks drive the commit structure.** Each task in `tasks.md` should map roughly to one commit. Makes `git log` readable and makes bisecting easier.

**Update specs when you're wrong.** If you discover a signal is actually signed, not unsigned, open a correction change rather than editing `openspec/specs/` directly. The audit trail matters.

**Explore liberally, commit carefully.** `/opsx:explore` costs nothing — use it whenever you're unsure about approach. The spec only gets written when you're confident.
