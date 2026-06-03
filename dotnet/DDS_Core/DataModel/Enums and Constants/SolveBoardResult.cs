namespace DDS_Core;

/// <summary>
/// DDS return codes for SolveBoard() and related functions.
/// </summary>
public enum SolveBoardResult
{
    /// <summary>
    /// Success - no fault detected.
    /// </summary>
    NoFault = 1,

    /// <summary>
    /// General error. Currently happens when fopen() fails or when AnalyseAllPlaysBin()
    /// gets a different number of Boards in its first two arguments.
    /// </summary>
    UnknownFault = -1,

    /// <summary>
    /// Zero cards supplied.
    /// </summary>
    ZeroCards = -2,

    /// <summary>
    /// Target exceeds number of tricks remaining.
    /// </summary>
    TargetTooHigh = -3,

    /// <summary>
    /// Cards duplicated.
    /// </summary>
    DuplicateCards = -4,

    /// <summary>
    /// Target is less than -1.
    /// </summary>
    TargetWrongLo = -5,

    /// <summary>
    /// Target is higher than 13.
    /// </summary>
    TargetWrongHi = -7,

    /// <summary>
    /// Solutions parameter is less than 1.
    /// </summary>
    SolutionsWrongLo = -8,

    /// <summary>
    /// Solutions parameter is higher than 3.
    /// </summary>
    SolutionsWrongHi = -9,

    /// <summary>
    /// Too many cards supplied.
    /// </summary>
    TooManyCards = -10,

    /// <summary>
    /// currentTrickSuit or currentTrickRank has wrong data.
    /// </summary>
    SuitOrRank = -12,

    /// <summary>
    /// Played card also remains in a hand.
    /// </summary>
    PlayedCard = -13,

    /// <summary>
    /// Wrong number of remaining cards in a hand.
    /// </summary>
    CardCount = -14,

    /// <summary>
    /// Thread index is not 0 .. maximum.
    /// </summary>
    ThreadIndex = -15,

    /// <summary>
    /// Mode parameter is less than 0.
    /// </summary>
    ModeWrongLo = -16,

    /// <summary>
    /// Mode parameter is higher than 2.
    /// </summary>
    ModeWrongHi = -17,

    /// <summary>
    /// Trump is not in 0 .. 4.
    /// </summary>
    TrumpWrong = -18,

    /// <summary>
    /// First is not in 0 .. 2.
    /// </summary>
    FirstWrong = -19,

    /// <summary>
    /// AnalysePlay input error (less than 0 or more than 52 cards,
    /// invalid suit or rank, or played card is not held by the right player).
    /// </summary>
    PlayFault = -98,

    /// <summary>
    /// PBN string error.
    /// </summary>
    PbnFault = -99,

    /// <summary>
    /// Too many Boards requested.
    /// </summary>
    TooManyBoards = -101,

    /// <summary>
    /// Could not create threads.
    /// </summary>
    ThreadCreate = -102,

    /// <summary>
    /// Something failed waiting for thread to end.
    /// </summary>
    ThreadWait = -103,

    /// <summary>
    /// Tried to set a multi-threading system that is not present in DLL.
    /// </summary>
    ThreadMissing = -104
}
