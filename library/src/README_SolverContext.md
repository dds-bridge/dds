# SolverContext scaffolding

This file documents the additive instance-based API introduced for the dynamic environment refactor.

- `SolverContext` wraps a `ThreadData*` for now and will later own all per-instance state.
- `SolveBoardWithContext` forwards to the existing `SolveBoardInternal` without changing behavior.
- Legacy APIs remain unchanged.

Include path: `#include "dds/SolverContext.h"` via the `include_prefix` on the `dds` libraries.
