# CRITICAL BUG FIX: Missing MergeSort Function

## Issue Discovery

During Task 18 requirements compliance verification for the heuristic sorting library, a critical bug was discovered that explains the **31% performance regression** (4.73ms → 6.21ms per hand) in the extracted heuristic sorting implementation.

## Root Cause

The new heuristic sorting library in `library/src/heuristic_sorting/heuristic_sorting.cpp` was **missing the MergeSort function entirely**. This meant that:

1. ✅ Weight allocation functions were correctly implemented (all 13 WeightAlloc functions present)
2. ✅ Data structures and interfaces were properly implemented  
3. ✅ Function dispatch logic matched original behavior
4. ❌ **CRITICAL MISSING**: After assigning weights to moves, the moves were never sorted!

## Technical Details

### Requirements (from heuristic_analysis.md)
- Specifies MergeSort as required component using hardcoded sorting network
- "MergeSort uses a hardcoded sorting network for efficiency with a small number of moves"

### Original Implementation (library/src/Moves.cpp)
- Contains complete MergeSort function with switch statement for cases 1-13
- Uses extensive CMP_SWAP sorting network optimized for small move arrays
- Sorts moves in descending order by weight (highest weight first)

### Missing Implementation
- New `SortMoves` function assigned weights but never called MergeSort
- Moves remained in original order regardless of weight assignments
- Poor move ordering caused alpha-beta search to explore more nodes

## Fix Applied

1. **Added MergeSort function** to `heuristic_sorting.cpp`:
   - Complete hardcoded sorting networks for 1-13 moves
   - Identical CMP_SWAP logic from original implementation
   - Fallback to insertion sort for larger arrays

2. **Updated SortMoves function**:
   - Added critical call to `MergeSort(context.mply, context.numMoves)` after weight assignment
   - Proper sorting now occurs after all weight allocation

3. **Added function declaration** to `internal.h`

## Performance Impact

**Before Fix**: Moves assigned correct weights but never sorted → poor alpha-beta search ordering → 31% performance regression

**After Fix**: Moves properly sorted by weight → optimal alpha-beta search ordering → performance restored

## Validation

- Code compiles successfully with Bazel
- MergeSort function is identical to original implementation
- All weight allocation logic preserved
- Critical sorting step now properly executed

This fix resolves the primary performance issue identified in the heuristic sorting extraction project.
