/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include "play_analyser.hpp"
#include <api/dll.h>
#include <api/analyse_play.hpp>
#include <solver_if.hpp>
#include <pbn.hpp>
#include <solver_context/solver_context.hpp>
#include <system/scheduler.hpp>
#include <system/system.hpp>
#include <vector>

using namespace std;


extern Scheduler scheduler;


/**
 * @brief Analyze a sequence of played cards (binary format) and determine the tricks taken.
 *
 * Creates an owned, single-use SolverContext and delegates to analyse_play(),
 * the context-aware modern entry point. thrId is retained for API
 * compatibility but ignored, as in the historical implementation.
 *
 * @param dl The Deal to analyze
 * @param play The sequence of played cards (binary format)
 * @param solvedp Pointer to result structure for solved play
 * @param thrId Index of thread to use (ignored)
 * @return 1 on success, error code otherwise
 */
int STDCALL AnalysePlayBin(
    Deal dl,
    PlayTraceBin play,
    SolvedPlay * solvedp,
    [[maybe_unused]] int thrId)
{
    SolverContext ctx;
    return analyse_play(ctx, dl, play, solvedp);
}


/**
 * @brief Analyze a sequence of played cards (PBN format) and determine the tricks taken.
 *
 * This function converts a PBN-format Deal and play trace to internal format,
 * then simulates play using double dummy analysis to determine the number of tricks won.
 *
 * @param dlPBN The Deal to analyze (PBN format)
 * @param playPBN The sequence of played cards (PBN format)
 * @param solvedp Pointer to result structure for solved play
 * @param thrId Index of thread to use
 * @return 1 on success, error code otherwise
 */
int STDCALL AnalysePlayPBN(
    DealPBN dlPBN,
    PlayTracePBN playPBN,
    SolvedPlay * solvedp,
    int thrId)
{
    Deal dl;
    PlayTraceBin play;

    if (convert_from_pbn(dlPBN.remainCards, dl.remainCards) !=
            RETURN_NO_FAULT)
        return RETURN_PBN_FAULT;

    dl.first = dlPBN.first;
    dl.trump = dlPBN.trump;
    for (int i = 0; i <= 2; i++)
    {
        dl.currentTrickSuit[i] = dlPBN.currentTrickSuit[i];
        dl.currentTrickRank[i] = dlPBN.currentTrickRank[i];
    }

    if (convert_play_from_pbn(playPBN, play) != RETURN_NO_FAULT)
        return RETURN_PLAY_FAULT;

    return AnalysePlayBin(dl, play, solvedp, thrId);
}


int STDCALL AnalyseAllPlaysBin(
    Boards const * bop,
    PlayTracesBin const * plp,
    SolvedPlays * solvedp,
    [[maybe_unused]] int chunkSize)
{
    if (bop->no_of_boards > MAXNOOFBOARDS)
        return RETURN_TOO_MANY_BOARDS;

    if (bop->no_of_boards != plp->no_of_boards)
        return RETURN_UNKNOWN_FAULT;

    scheduler.RegisterRun(RunMode::DDS_RUN_TRACE, * bop, * plp);

    START_BLOCK_TIMER;

    for (int bno = 0; bno < bop->no_of_boards; bno++) {
        SolvedPlay solved;
        const int res = AnalysePlayBin(bop->deals[bno], plp->plays[bno], &solved, 0);
        if (res == 1)
            solvedp->solved[bno] = solved;
        else
            return res;
    }

    END_BLOCK_TIMER;

    solvedp->no_of_boards = bop->no_of_boards;

#ifdef DDS_SCHEDULER
    scheduler.PrintTiming();
#endif

    return RETURN_NO_FAULT;
}


int STDCALL AnalyseAllPlaysPBN(
    BoardsPBN const * bopPBN,
    PlayTracesPBN const * plpPBN,
    SolvedPlays * solvedp,
    int chunkSize)
{
    Boards bd;
    PlayTracesBin pl;

    bd.no_of_boards = bopPBN->no_of_boards;
    if (bd.no_of_boards > MAXNOOFBOARDS)
        return RETURN_TOO_MANY_BOARDS;

    for (int k = 0; k < bopPBN->no_of_boards; k++)
    {
        Deal& dl = bd.deals[k];
        DealPBN const & dlp = bopPBN->deals[k];

        if (convert_from_pbn(dlp.remainCards,
                       dl.remainCards) != RETURN_NO_FAULT)
            return RETURN_PBN_FAULT;

        dl.trump = dlp.trump;
        dl.first = dlp.first;

        for (int i = 0; i <= 2; i++)
        {
            dl.currentTrickSuit[i] = dlp.currentTrickSuit[i];
            dl.currentTrickRank[i] = dlp.currentTrickRank[i];
        }
    }

    pl.no_of_boards = plpPBN->no_of_boards;

    for (int k = 0; k < plpPBN->no_of_boards; k++)
    {
        if (convert_play_from_pbn(plpPBN->plays[k], pl.plays[k]) !=
                RETURN_NO_FAULT)
            return RETURN_PLAY_FAULT;
    }

    chunkSize = 1;
    return AnalyseAllPlaysBin(&bd, &pl, solvedp, chunkSize);
}


void detect_play_duplicates(
    const Boards& bds,
    vector<int>& uniques,
    vector<int>& crossrefs)
{
    // This dummy function is there for consistency in System.cpp.
    // In practice there is not much point in deteting play repeats,
    // as it is highly unlikely that the play went identically at
    // two tables.

    uniques.resize(static_cast<unsigned>(bds.no_of_boards));
    crossrefs.resize(static_cast<unsigned>(bds.no_of_boards));
    for (unsigned i = 0; i < uniques.size(); i++)
    {
        uniques[i] = static_cast<int>(i);
        crossrefs[i] = -1;
    }
}



