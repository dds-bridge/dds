# Naming

For C and C++ code, use snake_case for functions, methods, variables, and parameters.

Prefer:
- `deal_fanout`
- `fanout_suit`
- `num_voids`

Avoid:
- `dealFanout`
- `fanoutSuit`
- `numVoids`

Exceptions:
- Match existing external or legacy APIs (for example, public C API names and types that already use a different style).
- Do not rename unrelated legacy identifiers in the same change unless the task requires it.
