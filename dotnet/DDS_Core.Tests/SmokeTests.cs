using DDS_Core;

namespace DDS_Core.Tests;

/// <summary>
/// End-to-end solving through the retargeted binding — the .NET analogue of
/// <c>DdsSmokeTest.java</c>. These are what prove the <c>dds_c_*</c> entry
/// points actually resolve and marshal correctly on a non-Windows platform.
/// </summary>
public class SmokeTests
{
    [Fact]
    public void SolveBoard_ReferenceDeal_TakesThirteenTricks()
    {
        using var ctx = new SolverContext();

        ctx.SolveBoard(TestDeals.Reference(), -1, 1, 1, out FutureTricks fut);

        Assert.Equal(TestDeals.ExpectedTricks, fut.Score[0]);
    }

    [Fact]
    public void CalcDdTable_ReferenceDeal_MatchesExpectedTable()
    {
        using var ctx = new SolverContext();
        var deal = TestDeals.ReferenceTable();

        ctx.CalcDdTable(deal, out DdTableResults results);

        for (int strain = 0; strain < 5; strain++)
            for (int hand = 0; hand < 4; hand++)
                Assert.Equal(TestDeals.ExpectedDdTable[strain][hand], results.ResultsTable[strain, hand]);
    }

    /// <summary>
    /// The PBN twin must agree with the binary form. This is the only PBN pair
    /// on the modern layer, added to the shim by this work.
    /// </summary>
    [Fact]
    public void CalcDdTable_PbnAgreesWithBinary()
    {
        using var ctx = new SolverContext();

        ctx.CalcDdTable(TestDeals.ReferenceTable(), out DdTableResults binary);

        var pbnDeal = new DdTableDealPBN { Cards = TestDeals.ReferencePbn };
        ctx.CalcDdTable(pbnDeal, out DdTableResults pbn);

        for (int strain = 0; strain < 5; strain++)
            for (int hand = 0; hand < 4; hand++)
                Assert.Equal(binary.ResultsTable[strain, hand], pbn.ResultsTable[strain, hand]);
    }

    [Fact]
    public void CalcPar_ReferenceDeal_ProducesNonEmptyScore()
    {
        using var ctx = new SolverContext();

        ctx.CalcPar(TestDeals.ReferenceTable(), 0 /* vulnerable: none */,
                    out DdTableResults _, out ParResults par);

        Assert.False(string.IsNullOrWhiteSpace(par.ParScores[0]));
    }
}
