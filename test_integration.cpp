#include "library/src/heuristic_sorting/heuristic_sorting.h"

// Test that the CallHeuristic function signature is correctly defined
// This should compile without errors if the integration is working

void test_callheuristic_signature() {
    // Use the actual types that are forward declared in the header
    pos* tpos = nullptr;
    moveType* bestMove = nullptr;
    moveType* bestMoveTT = nullptr;
    relRanksType* thrp_rel = nullptr;
    moveType* mply = nullptr;
    trackType* trackp = nullptr;
    
    // This should compile if the signature is correct
    // (though it would crash at runtime due to null pointers)
    if (false) {  // Never actually execute
        CallHeuristic(
            *tpos,
            *bestMove,
            *bestMoveTT,
            thrp_rel,
            mply,
            10,      // numMoves
            5,       // lastNumMoves
            0,       // trump
            1,       // suit
            trackp,  // trackp
            2,       // currTrick
            1,       // currHand
            0,       // leadHand
            2        // leadSuit
        );
    }
}
