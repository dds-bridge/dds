using DDS_Core;

namespace DDS_Core.Tests;

/// <summary>
/// The shared reference board, matching <c>DdsSmokeTest.java</c> and
/// <c>dds_c_api_test.cpp</c> so the JVM, C++, and .NET bindings all assert
/// against one fixture.
/// </summary>
internal static class TestDeals
{
    /// <summary>Full 13-card holding bitmask (ranks 2..A).</summary>
    internal const uint FullSuit = 0x7FFC;

    /// <summary>
    /// North holds all spades, East all hearts, South all diamonds, West all
    /// clubs. With spades trump and North to lead, North/South take all 13.
    /// </summary>
    internal const int ExpectedTricks = 13;

    /// <summary>res_table[strain][hand] for the reference board.</summary>
    internal static readonly int[][] ExpectedDdTable =
    [
        [13, 0, 13, 0],   // spades
        [0, 13, 0, 13],   // hearts
        [13, 0, 13, 0],   // diamonds
        [0, 13, 0, 13],   // clubs
        [0, 0, 0, 0],     // no-trump
    ];

    /// <summary>The same board in PBN: spades.hearts.diamonds.clubs per hand.</summary>
    internal const string ReferencePbn =
        "N:AKQJT98765432... .AKQJT98765432.. ..AKQJT98765432. ...AKQJT98765432";

    internal static Deal Reference()
    {
        var deal = new Deal { Trump = 0, First = 0, RemainingCards = new FourHands() };
        deal.RemainingCards[0, 0] = FullSuit;   // North spades
        deal.RemainingCards[1, 1] = FullSuit;   // East hearts
        deal.RemainingCards[2, 2] = FullSuit;   // South diamonds
        deal.RemainingCards[3, 3] = FullSuit;   // West clubs
        return deal;
    }

    internal static DdTableDeal ReferenceTable()
    {
        var deal = new DdTableDeal { Cards = new FourHands() };
        deal.Cards[0, 0] = FullSuit;
        deal.Cards[1, 1] = FullSuit;
        deal.Cards[2, 2] = FullSuit;
        deal.Cards[3, 3] = FullSuit;
        return deal;
    }
}
