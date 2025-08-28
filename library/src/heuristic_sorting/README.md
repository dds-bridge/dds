# Heuristic Sorting Library

This library provides a deterministic, side-effect-free move ordering heuristic for the game of bridge.

## Purpose

The primary purpose of this library is to encapsulate the complex move ordering and weighting logic from the main DDS engine into a standalone, testable, and reusable component. The original implementation in `Moves.cpp` mixed move generation, weighting, and sorting with the core alpha-beta search logic, making it difficult to maintain and test in isolation.

This library extracts the heuristic into a pure function, `score_and_order`, which takes the complete game state as input and returns a sorted list of moves based on their calculated weights.

## Algorithm Mapping

The logic implemented in this library is a direct port of the original heuristic found in `Moves.cpp`. The high-level concepts and weighting strategies are described in the document `doc/heuristic-sorting.md`. The core functions from the original implementation, such as the `WeightAlloc...` family of functions, have been ported to the `heuristic_sorting.cpp` file and are called from the main `score_and_order` function.

The data structures used, such as `Pos`, `RelRanksType`, and `HeuristicContext`, are designed to provide the minimal necessary interface for the heuristic to perform its calculations, as outlined in the `heuristic_analysis.md` document.

By maintaining a 1:1 mapping of the logic, this library achieves parity with the original implementation while providing a much cleaner and more maintainable architecture.
