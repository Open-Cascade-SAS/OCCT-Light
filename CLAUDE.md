# CLAUDE.md — OCCT-Light

This file is read by [Claude Code](https://claude.com/claude-code) at session start. It imports the cross-agent canonical guide and adds a few Claude-Code-specific notes.

@AGENTS.md

---

## Claude-Code-specific notes

These supplement (do not override) `AGENTS.md`.

### Tools and where they shine

- **Read** for known paths, **Bash `grep`/`find`** for known symbols. Reserve **Agent** (subagent) for open-ended exploration that would take more than 3 sequential reads. Cap at 3 parallel Explore agents per question and brief each one with a specific focus area.
- Use **Plan** subagent only when the user has explicitly asked for a plan — for ordinary edits, write directly.
- **TaskCreate / TaskUpdate** for any work that takes more than ~3 distinct steps; mark each task `completed` immediately when done. Don't batch.
- `/ultrareview` is user-triggered only — never spawn it from a tool.

### Don't run `/ultrareview`, `/security-review`, or `/review` autonomously

These are billed and user-triggered. Suggest them; never invoke them yourself.

### Legacy MVP reference

If a sibling `light-occt/` MVP repository exists, treat it as read-only reference; do not modify it from this project workflow. This repository is a redesign and does not aim for source compatibility with the MVP.

### Subagent picks for this project

- Use **Explore** for "where is X" / "show me how the existing code does Y" questions across `src/`, `include/`, or the OCCT tree. Brief: a specific focus, expected output format, word cap.
- Use **Plan** when the user asks for a plan or when a change touches more than two modules.
- Use **claude-code-guide** when the user asks how a Claude Code feature itself works.

### Don't do these without asking

- Run `cmake --build --preset full` from scratch — it pulls in OCCT and gtest fetches and is slow. Use `core-only` for foundation iteration; use targeted module builds otherwise.
- Modify `LICENSE_AGPL_30.txt`.
- Add a new design doc to `docs/design/` (the six existing ones are the design surface; new files fragment intent).
- Touch personal files outside the repository.
