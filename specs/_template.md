---
capability: <kebab-case-name>
owners: [<top-level dirs or components, e.g. parser, fixed_array]
last-updated: <YYYY-MM-DD>
related-plans: [<plan-name>, ...]
---

# <Capability Title>

> **Specs vs. doxygen — the litmus test.** If a fact lives naturally in one
> symbol's doc comment (a signature, a `@param`, a per-function note), it belongs
> in **doxygen**, not here. Specs hold only what doxygen structurally cannot:
> capability-wide intent, cross-symbol invariants, and deliberate non-goals.
> Do **not** re-list the per-symbol API — point to the doxygen page instead.

## Purpose

One short paragraph: what this capability provides and to whom. Intent and the
"why it exists" framing live here — doxygen has nowhere to put them.

## Behaviour & invariants

> Per-symbol signatures live in doxygen (`<doxygen-page>.html`). This section
> records only facts that span the whole capability or that a reader cannot infer
> from a single symbol's signature.

- Cross-cutting guarantees the code upholds today (e.g. "on failure the input is
  restored", "round-trip: write then read returns the original value").
- Things consumers may rely on; assumptions that must remain true.
- Non-obvious behaviours no single symbol owns (stall guards, void-type gating,
  lock-step delegation, error-type conventions).
- Keep these current — `action-tasks` updates them when a plan's "Spec impact"
  names this file. (Per-API drift is doxygen's job, not yours.)

## Key entry points

- `path/to/file.h` — `function_or_type()` — one-line role.
  Doxygen: `<doxygen-page>.html`.

## Known gaps / non-goals

- What this capability deliberately does not do, and what is intentionally
  unimplemented (doxygen cannot express "deliberately absent").
