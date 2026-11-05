#include <api/analyse_play.hpp>
#include <solver_if.hpp>

/**
 * @brief Analyze a sequence of played cards (binary format) and determine the tricks taken.
 *
 * This function simulates play of a bridge Deal according to the provided play trace,
 * using double dummy analysis to determine the number of tricks won at each step.
 *
 * @param ctx Solver context containing state and resources, reused across the
 *            initial solve and every subsequent per-card analyse_later_board
 *            call so the hint-bounded incremental searches see a warm TT.
 * @param dl_in The Deal to analyze
 * @param play The sequence of played cards (binary format)
 * @param solvedp Pointer to result structure for solved play
 * @return RETURN_NO_FAULT on success, error code otherwise
 */
auto analyse_play(
    SolverContext& ctx,
    const Deal& dl_in,
    const PlayTraceBin& play,
    SolvedPlay* solvedp) -> int
{
    // Validate every claimed play.suit/play.rank entry before any of it is
    // used to index bit_map_rank[] or dl.remainCards[player][suit] below --
    // those arrays are sized for valid suits/ranks only, so an out-of-range
    // value is an out-of-bounds access, not something a caller could catch
    // as a C++ exception.
    if (play.number < 0 || play.number > 52)
        return RETURN_PLAY_FAULT;
    for (int i = 0; i < play.number; i++)
    {
        if (play.suit[i] < 0 || play.suit[i] >= DDS_SUITS)
            return RETURN_PLAY_FAULT;
        if (play.rank[i] < 2 || play.rank[i] > 14)
            return RETURN_PLAY_FAULT;
    }

    Deal dl = dl_in;

    MoveType move;
    FutureTricks fut;

    int ret = solve_board_internal(ctx, dl, -1, 1, 1, &fut);
    if (ret != RETURN_NO_FAULT)
        return ret;
    const int ini_depth = ctx.search().ini_depth();
    const int numTricks = ((ini_depth + 3) >> 2) + 1;
    const int numCardsPlayed = ((48 - ini_depth) % 4) + 1;

    int last_trick = (play.number + 3) / 4;
    int last_card = ((play.number + 3) % 4) + 1;
    if (last_trick >= numTricks)
    {
        last_trick = numTricks-1;
        last_card = 4;
    }
    solvedp->number = 0;

    solvedp->tricks[0] = (numCardsPlayed % 2 == 1 ?
        numTricks - fut.score[0] : fut.score[0]);
    int hint = solvedp->tricks[0];
    int hintDir;

    int running_remainder = numTricks;
    int running_declarer = 0;
    int running_player = dl.first;
    int running_side = 1; /* defenders */
    int start_side = running_player % 2;

    for (int trick = 1; trick <= last_trick; trick++)
    {
        int best_card = 0, best_suit = 0, best_player = 0, trump_played = 0;
        int lc = (trick == last_trick ? last_card : 4);

        bool haveCurrent = (numCardsPlayed > 1 && trick == 1);
        int offset = 4 * (trick - 1) - (numCardsPlayed - 1);

        for (int card = 1; card <= lc; card++)
        {
            int suit, rr;
            bool usingCurrent = (haveCurrent && card < numCardsPlayed);
            if (usingCurrent)
            {
                suit = dl.currentTrickSuit[card - 1];
                rr = dl.currentTrickRank[card - 1];
            }
            else
            {
                suit = play.suit[offset + card - 1];
                rr = play.rank[offset + card - 1];
            }
            unsigned hold = static_cast<unsigned>(bit_map_rank[rr] << 2);

            move.suit = suit;
            move.rank = rr;
            move.sequence = rr;

            /* Keep track of the winner of the trick so far */
            if (card == 1)
            {
                best_card = rr;
                best_suit = suit;
                best_player = dl.first;
                trump_played = (suit == dl.trump ? 1 : 0);
            }
            else if (suit == dl.trump)
            {
                if (! trump_played || rr > best_card)
                {
                    best_card = rr;
                    best_suit = suit;
                    best_player = running_player;
                    trump_played = 1;
                }
            }
            else if (! trump_played && suit == best_suit && rr > best_card)
            {
                best_card = rr;
                best_player = running_player;
            }

            if ((dl.remainCards[running_player][suit] & hold) == 0)
            {
                if (! usingCurrent)
                    return RETURN_PLAY_FAULT;
            }
            else
                dl.remainCards[running_player][suit] ^= hold;

            if (card == 4)
            {
                running_declarer += (best_player % 2 == start_side ? 0 : 1);
                running_remainder--;

                if ((dl.first + best_player) % 2 == 0)
                {
                    hintDir = 0; // Same side leads again; lower bound
                    hint = running_remainder - fut.score[0];
                }
                else
                {
                    hintDir = 1; // Other ("our") side wins trick; upper bound
                    hint = fut.score[0] - 1;
                }

                dl.first = best_player;
                running_side = (dl.first % 2 == start_side ? 1 : 0);
                running_player = dl.first;
            }
            else
            {
                running_player = (running_player + 1) % 4;
                running_side = 1 - running_side;
                hint = running_remainder - fut.score[0];
                hintDir = 0;
            }

            if (usingCurrent)
                continue;

            if ((ret = analyse_later_board(ctx, dl.first, &move, hint,
                hintDir, &fut))
                    != RETURN_NO_FAULT)
            {
                return ret;
            }

            int new_solved_decl = running_declarer + (running_side ?
                running_remainder - fut.score[0] : fut.score[0]);

            solvedp->tricks[offset + card] = new_solved_decl;
        }
    }
    solvedp->number = 4 * last_trick + last_card - 3 - (numCardsPlayed - 1);

    return RETURN_NO_FAULT;
}
