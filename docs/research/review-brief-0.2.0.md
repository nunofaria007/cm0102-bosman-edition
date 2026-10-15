# Review brief, 0.2.0 (2026-10-15)

Two Opus review agents read this brief. Each gets one angle. Both read the same code.

## What to review

- Worktree: `/Users/max/Projects/cm0102/cm0102-mac/.claude/worktrees/agent-a069b80021b6499a7`, branch `worktree-agent-a069b80021b6499a7`.
- The diff: `git diff v0.1.0..HEAD` in that worktree (38 files). Leave out `docs/` except where the code names a doc as its evidence.
- Spec: `docs/specs/2026-10-14-quality-of-life-0.2.0-spec.md`. Terms: `GLOSSARY.md`. Repo rules: `CLAUDE.md`, `docs/HANDOVER.md` ("Rules").
- Not in 0.2.0: the assistant A key and the test speed-ups (wait steps, `tests/fixtures/`). Do not report them missing.

## Angles

1. **Spec agent.** Does each 0.2.0 feature do what the spec says, with the option and default the spec gives? Can any feature change a match result or a game decision (the hard rule)? Does each option off give the original's behaviour?
2. **Standards agent.** Correctness bugs: memory reads and writes out of range, registers or stack not kept across `rt_callback`, wraps that act on the wrong call site, state left set after a screen closes, Windows and Mac differences, thread safety in the sound code. Then repo standards: comments in plain STE English, every address with its evidence, no original code or data in git.

## Rules

- Read only. Do not edit files, do not commit, do not build, do not run the game. A test game in parallel breaks the timed test run in progress.
- Report each finding with file and line, what goes wrong, a concrete case, and a severity: must fix before release, should fix, or note. Mark an inference as an inference.
- Report only what you can show from the code. Write "unknown" rather than guess.
- At most 300 words in the final reply, findings most severe first.
