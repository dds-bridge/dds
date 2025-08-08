# GitHub Copilot Instructions — Multi-Language Rules

## 0. Personal Developer Profile
- Assume I am a **senior developer** who values clarity, maintainability, and modern best practices.
- When multiple solutions exist, choose the **most idiomatic** approach for the language.
- Avoid deprecated APIs and outdated patterns.
- Write code that is self-documenting; use comments only for complex logic.

---

## 1. Swift / iOS
- Target **iOS 17+**, **Swift 5.10+**.
- Prefer **Swift Concurrency** (`async/await`, `Task`) over GCD or completion handlers.
- Follow **MVVM** for UI architecture.
- Use `struct` for value types; use `class` only when reference semantics are needed.
- Apply **Swift API Design Guidelines**:
  - Descriptive parameter labels.
  - Omit needless words.
- Avoid force unwrapping unless absolutely safe.
- All new code must pass **SwiftLint** rules in `.swiftlint.yml`.
- Write **unit tests** using XCTest for all public functions.

---

## 2. Python
- Target **Python 3.12+**.
- Follow **PEP8** and use type hints for all functions.
- Prefer `dataclasses` over `namedtuple` for structured data.
- Use f-strings for formatting.
- Avoid mutable default arguments.
- For async work, use `asyncio` with `async/await`.

---

## 3. C++
- Use **C++20** standard.
- Prefer `std::unique_ptr` and `std::shared_ptr` over raw pointers.
- Use `const` correctness consistently.
- Prefer range-based for loops and `<algorithm>` functions.
- Avoid macros except for header guards (use `#pragma once` if allowed).
- Use RAII for resource management.

---

## 4. JavaScript / TypeScript
- Prefer **TypeScript** with strict mode enabled.
- Follow ESLint and Prettier rules from the project config.
- Use `const` and `let` instead of `var`.
- Use modern ES modules (`import/export`) and async/await for async code.

---

## 5. Project Context (Optional)
- **Build system**: [your build system, e.g., Xcode + SPM]
- **CI/CD**: [e.g., GitHub Actions running lint/tests]
- **Key dependencies**: [list important frameworks/libraries]
- **Domain-specific terminology**: [list important terms for Copilot to know]

---

**Reminder to Copilot:**  
Always apply the relevant per-language section depending on the file’s language. Follow personal profile rules in all cases.
