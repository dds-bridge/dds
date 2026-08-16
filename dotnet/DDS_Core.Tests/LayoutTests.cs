using System.Runtime.InteropServices;
using DDS_Core;

namespace DDS_Core.Tests;

/// <summary>
/// Pins the managed struct layouts to the C structs in library/src/api/dll.h.
/// </summary>
/// <remarks>
/// <para>
/// These matter more than they look. The managed types were only ever exercised
/// against the MSVC ABI; a layout mismatch on SysV (Linux) or AArch64 (Apple
/// Silicon) corrupts results <i>silently</i> rather than throwing, so the smoke
/// tests alone cannot catch it.
/// </para>
/// <para>
/// The expected values are derived from the C headers — compiled with
/// <c>offsetof</c>/<c>sizeof</c> — not from running the C# side. Asserting what
/// the managed code already does would prove nothing.
/// </para>
/// </remarks>
public class LayoutTests
{
    // ---- Ground truth from library/src/api/dll.h (offsetof/sizeof, LP64) ----

    [Fact]
    public void Deal_MatchesNativeLayout()
    {
        Assert.Equal(96, Marshal.SizeOf<Deal>());
        Assert.Equal(0, (int) Marshal.OffsetOf<Deal>(nameof(Deal.Trump)));
        Assert.Equal(4, (int) Marshal.OffsetOf<Deal>(nameof(Deal.First)));
        Assert.Equal(8, (int) Marshal.OffsetOf<Deal>(nameof(Deal.CurrentTrickSuit)));
        Assert.Equal(20, (int) Marshal.OffsetOf<Deal>(nameof(Deal.CurrentTrickRank)));
        Assert.Equal(32, (int) Marshal.OffsetOf<Deal>(nameof(Deal.RemainingCards)));
    }

    [Fact]
    public void FutureTricks_MatchesNativeLayout()
    {
        Assert.Equal(216, Marshal.SizeOf<FutureTricks>());
        Assert.Equal(0, (int) Marshal.OffsetOf<FutureTricks>(nameof(FutureTricks.Nodes)));
        Assert.Equal(4, (int) Marshal.OffsetOf<FutureTricks>(nameof(FutureTricks.NumberOfCards)));
        Assert.Equal(8, (int) Marshal.OffsetOf<FutureTricks>(nameof(FutureTricks.Suit)));
        Assert.Equal(60, (int) Marshal.OffsetOf<FutureTricks>(nameof(FutureTricks.Ranks)));
        Assert.Equal(112, (int) Marshal.OffsetOf<FutureTricks>(nameof(FutureTricks.EqualGroups)));
        Assert.Equal(164, (int) Marshal.OffsetOf<FutureTricks>(nameof(FutureTricks.Score)));
    }

    [Fact]
    public void DdTableDeal_MatchesNativeLayout()
        => Assert.Equal(64, Marshal.SizeOf<DdTableDeal>());

    [Fact]
    public void DdTableResults_MatchesNativeLayout()
        => Assert.Equal(80, Marshal.SizeOf<DdTableResults>());

    [Fact]
    public void ParResults_MatchesNativeLayout()
        => Assert.Equal(288, Marshal.SizeOf<ParResults>());

    [Fact]
    public void DdTableDealPBN_MatchesNativeLayout()
        => Assert.Equal(80, Marshal.SizeOf<DdTableDealPBN>());

    /// <summary>
    /// remainCards is [hand][suit], row-major with DDS_SUITS = 4 columns, so
    /// element [hand][suit] lives at index hand * 4 + suit. The JVM binding
    /// relies on the same arithmetic; if FourHands ever disagreed, deals would
    /// be silently transposed rather than rejected.
    /// </summary>
    [Fact]
    public void FourHands_IsRowMajorByHandThenSuit()
    {
        var hands = new FourHands();
        for (int hand = 0; hand < 4; hand++)
            for (int suit = 0; suit < 4; suit++)
                hands[hand, suit] = (uint) (hand * 4 + suit);

        var flat = hands.AsSpan();
        for (int i = 0; i < FourHands.SIZE; i++)
            Assert.Equal((uint) i, flat[i]);
    }
}
