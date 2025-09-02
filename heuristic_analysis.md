## Move Ordering Heuristic Analysis

This document outlines the functions and data structures used in the move ordering heuristic in `library/src/Moves.cpp`.

### Core Functions

The primary functions involved in generating and ordering moves are:

-   **`MoveGen0`**: Generates and weights moves for the leader of a trick.
-   **`MoveGen123`**: Generates and weights moves for the followers in a trick.
-   **`WeightAlloc...` functions**: This is a family of functions that implement the core heuristic logic for assigning weights to moves. The choice of function depends on the game state (trump or no-trump) and the player's position in the trick.
    -   `WeightAllocTrump0`
    -   `WeightAllocNT0`
    -   `WeightAllocTrumpNotvoid1`
    -   `WeightAllocNTNotvoid1`
    -   `WeightAllocTrumpVoid1`
    -   `WeightAllocNTVoid1`
    -   `WeightAllocTrumpNotvoid2`
    -   `WeightAllocNTNotvoid2`
    -   `WeightAllocTrumpVoid2`
    -   `WeightAllocNTVoid2`
    -   `WeightAllocCombinedNotvoid3`
    -   `WeightAllocTrumpVoid3`
    -   `WeightAllocNTVoid3`
-   **`MergeSort`**: This function sorts the generated moves based on the weights assigned by the `WeightAlloc...` functions. It uses a hardcoded sorting network for efficiency with a small number of moves.

### Key Data Structures

The heuristic relies on several data structures to represent the game state and moves:

-   **`moveType`**: Represents a single potential move. It contains:
    -   `suit`: The suit of the card.
    -   `rank`: The rank of the card.
    -   `sequence`: A bitmask representing a sequence of cards.
    -   `weight`: The heuristic weight assigned to the move.

-   **`movePlyType`**: A container for all possible moves for a player in a single trick.

-   **`pos`**: Represents the complete state of the cards at a given point in the game. It includes:
    -   `rankInSuit[DDS_HANDS][DDS_SUITS]`: A bitmask of the cards held by each player in each suit.
    -   `length[DDS_HANDS][DDS_SUITS]`: The number of cards held by each player in each suit.
    -   `aggr[DDS_SUITS]`: An aggregate bitmask of all cards in a suit.
    -   `winner[DDS_SUITS]`: The winning card in each suit.
    -   `secondBest[DDS_SUITS]`: The second-best card in each suit.

-   **`trackType`**: Tracks the progress of the current trick. It includes:
    -   `leadHand`: The hand that led the trick.
    -   `leadSuit`: The suit that was led.
    -   `removedRanks[DDS_SUITS]`: A bitmask of cards that have already been played.
    -   `move[DDS_HANDS]`: The moves made so far in the trick.
    -   `high[DDS_HANDS]`: The winning card so far in the trick.
    -   `playSuits[DDS_HANDS]`: The suit of the card played by each hand.
    -   `playRanks[DDS_HANDS]`: The rank of the card played by each hand.

-   **`relRanksType`**: Contains pre-calculated information about the relative ranks of cards.

### Minimal Data Interface for a New Library

To create a new, independent library for the move ordering heuristic, the following data needs to be passed in as a minimal interface:

-   The current player's hand and the hands of the other three players (`pos.rankInSuit`).
-   The trump suit.
-   The current trick number.
-   The hand that led the trick (`trackType.leadHand`).
-   The suit that was led (`trackType.leadSuit`).
-   The cards that have been played in the current trick (`trackType.move`).
-   A bitmask of all cards played in previous tricks (`trackType.removedRanks`).
-   Information about the winning and second-best cards in each suit (`pos.winner`, `pos.secondBest`).
-   The cards held by each player (`pos.rankInSuit`).
-   The length of each suit for each player (`pos.length`).
-   The aggregate ranks in each suit (`pos.aggr`).
-   Relative rank information (`relRanksType`).
-   Best moves from transposition tables (`bestMove`, `bestMoveTT`).
