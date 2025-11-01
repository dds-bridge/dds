# SolverContext scaffolding

This file documents the additive instance-based API introduced for the dynamic environment refactor.

- `SolverContext` stores a `std::shared_ptr<ThreadData>` and may either own
	per-context thread state or act as a non-owning facade via a
	non-owning `std::shared_ptr<ThreadData>` (constructed with a no-op
	deleter). The codebase is migrating to instance-scoped ownership.
- `SolveBoardWithContext` forwards to the existing `SolveBoardInternal` without changing behavior.
- Legacy APIs remain unchanged.

Include path: `#include "dds/SolverContext.h"` via the `include_prefix` on the `dds` libraries.
