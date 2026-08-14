using DDS_Core;

namespace DdTableForDeal.Tests;

public class ParseVulnerableTests
{
    [Theory]
    [InlineData("none", 0)]
    [InlineData("None", 0)]
    [InlineData("0", 0)]
    [InlineData("both", 1)]
    [InlineData("1", 1)]
    [InlineData("ns", 2)]
    [InlineData("NS", 2)]
    [InlineData("2", 2)]
    [InlineData("ew", 3)]
    [InlineData("3", 3)]
    public void AcceptsAliasesAndCodes(string text, int expected)
    {
        Assert.Equal(expected, DdTableForDealLib.ParseVulnerable(text));
    }

    [Theory]
    [InlineData("")]
    [InlineData("maybe")]
    [InlineData("4")]
    public void RejectsUnknown(string text)
    {
        Assert.Null(DdTableForDealLib.ParseVulnerable(text));
    }
}

public class ParseLimitTests
{
    [Theory]
    [InlineData("1", 1u)]
    [InlineData("25", 25u)]
    public void AcceptsPositiveIntegers(string text, uint expected)
    {
        Assert.Equal(expected, DdTableForDealLib.ParseLimit(text));
    }

    [Theory]
    [InlineData("")]
    [InlineData("0")]
    [InlineData("-1")]
    [InlineData("3x")]
    [InlineData("1.5")]
    public void RejectsNonPositiveAndNonNumeric(string text)
    {
        Assert.Null(DdTableForDealLib.ParseLimit(text));
    }
}

public class ApplyDealLimitTests
{
    [Fact]
    public void KeepsPrefixWhenLimited()
    {
        string[] deals = ["a", "b", "c"];
        Assert.Equal(["a", "b"], DdTableForDealLib.ApplyDealLimit(deals, 2));
        Assert.Equal(deals, DdTableForDealLib.ApplyDealLimit(deals, null));
        Assert.Equal(deals, DdTableForDealLib.ApplyDealLimit(deals, 10));
    }
}

public class ExtractDealTagsTests
{
    [Fact]
    public void FindsAllTags()
    {
        const string text =
            "{Board 1}\n"
            + "[Deal \"N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 "
            + "5.A95432.7632.K6 AKJ9842.K.T8.J93\"]\n"
            + "\n"
            + "{Board 2}\n"
            + "[Deal \"N:QJ6.K652.J85.T98 873.J97.AT764.Q4 "
            + "K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3\"]\n";

        var deals = DdTableForDealLib.ExtractDealTags(text);
        Assert.Equal(2, deals.Count);
        Assert.Equal(
            "N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 "
            + "5.A95432.7632.K6 AKJ9842.K.T8.J93",
            deals[0]);
        Assert.Equal(
            "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 "
            + "K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3",
            deals[1]);
    }

    [Fact]
    public void EmptyWhenNoTags()
    {
        Assert.Empty(DdTableForDealLib.ExtractDealTags("{comment only}"));
    }
}

public class UniqueDealsTests
{
    [Fact]
    public void PreservesFirstSeenOrderAndDropsDuplicates()
    {
        string[] deals = ["deal-a", "deal-b", "deal-a", "deal-c", "deal-b", "deal-a"];
        var unique = DdTableForDealLib.UniqueDeals(deals);
        Assert.Equal(["deal-a", "deal-b", "deal-c"], unique);
    }
}

public class LooksLikePathTests
{
    [Fact]
    public void DetectsPathsAndExtensions()
    {
        Assert.True(DdTableForDealLib.LooksLikePath("boards.pbn"));
        Assert.True(DdTableForDealLib.LooksLikePath("hands/x.pbn"));
        Assert.True(DdTableForDealLib.LooksLikePath("notes.txt"));
        Assert.False(DdTableForDealLib.LooksLikePath(
            "N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 "
            + "5.A95432.7632.K6 AKJ9842.K.T8.J93"));
    }
}

public class ParseCliTests
{
    private const string ExampleDeal =
        "N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 "
        + "5.A95432.7632.K6 AKJ9842.K.T8.J93";

    [Fact]
    public void DealOnlyDefaultsVulnerableToNone()
    {
        var parsed = DdTableForDealLib.ParseCli(["prog", ExampleDeal]);
        Assert.NotNull(parsed);
        Assert.Equal(ExampleDeal, parsed.Value.DealArg);
        Assert.Equal(0, parsed.Value.Vulnerable);
        Assert.Null(parsed.Value.Limit);
    }

    [Fact]
    public void VulFlagBeforeDeal()
    {
        var parsed = DdTableForDealLib.ParseCli(["prog", "--vul", "ns", ExampleDeal]);
        Assert.NotNull(parsed);
        Assert.Equal(ExampleDeal, parsed.Value.DealArg);
        Assert.Equal(2, parsed.Value.Vulnerable);
        Assert.Null(parsed.Value.Limit);
    }

    [Fact]
    public void LimitAndVulTogether()
    {
        var parsed = DdTableForDealLib.ParseCli(
            ["prog", "--vul", "ns", "--limit", "1", "boards.pbn"]);
        Assert.NotNull(parsed);
        Assert.Equal("boards.pbn", parsed.Value.DealArg);
        Assert.Equal(2, parsed.Value.Vulnerable);
        Assert.Equal(1u, parsed.Value.Limit);
    }

    [Fact]
    public void HelpReturnsNull()
    {
        Assert.Null(DdTableForDealLib.ParseCli(["prog", "--help"]));
        Assert.Null(DdTableForDealLib.ParseCli(["prog", "-h"]));
    }

    [Fact]
    public void UnknownOptionThrows()
    {
        Assert.Throws<ArgumentException>(() =>
            DdTableForDealLib.ParseCli(["prog", "--nope", ExampleDeal]));
    }
}

public class FormatParLineTests
{
    private static ContractType MakeContract(
        int seats, int level, int denom, int underTricks, int overTricks) =>
        new()
        {
            Seats = seats,
            Level = level,
            Denomination = denom,
            UnderTricks = underTricks,
            OverTricks = overTricks,
        };

    [Fact]
    public void SingleSacrifice()
    {
        var sides = new ParResultsMaster[2];
        sides[0].Score = -300;
        sides[0].Number = 1;
        sides[0].Contracts[0] = MakeContract(/*NS*/ 4, 5, /*H*/ 2, 2, 0);
        sides[1].Score = 300;
        sides[1].Number = 1;
        sides[1].Contracts[0] = MakeContract(/*NS*/ 4, 5, /*H*/ 2, 2, 0);

        Assert.Equal("Par: NS 5Hx -2 -300", DdTableForDealLib.FormatParLine(sides));
    }

    [Fact]
    public void SingleMakingUsesEqualsAndDeclaringScore()
    {
        var sides = new ParResultsMaster[2];
        sides[0].Score = -110;
        sides[0].Number = 1;
        sides[0].Contracts[0] = MakeContract(/*EW*/ 5, 2, /*S*/ 1, 0, 0);
        sides[1].Score = 110;
        sides[1].Number = 1;
        sides[1].Contracts[0] = MakeContract(/*EW*/ 5, 2, /*S*/ 1, 0, 0);

        Assert.Equal("Par: EW 2S = 110", DdTableForDealLib.FormatParLine(sides));
    }

    [Fact]
    public void MultipleSacrificesOnOneLine()
    {
        var sides = new ParResultsMaster[2];
        sides[0].Score = 100;
        sides[0].Number = 2;
        sides[0].Contracts[0] = MakeContract(/*EW*/ 5, 3, /*D*/ 3, 1, 0);
        sides[0].Contracts[1] = MakeContract(/*EW*/ 5, 3, /*C*/ 4, 1, 0);
        sides[1].Score = -100;
        sides[1].Number = 2;
        sides[1].Contracts[0] = MakeContract(/*EW*/ 5, 3, /*D*/ 3, 1, 0);
        sides[1].Contracts[1] = MakeContract(/*EW*/ 5, 3, /*C*/ 4, 1, 0);

        Assert.Equal("Par: EW 3Dx, 3Cx -1 -100", DdTableForDealLib.FormatParLine(sides));
    }

    [Fact]
    public void OmitsRepeatedDeclaringSideWhenSeatsDiffer()
    {
        var sides = new ParResultsMaster[2];
        sides[0].Score = 100;
        sides[0].Number = 2;
        sides[0].Contracts[0] = MakeContract(/*EW*/ 5, 4, /*H*/ 2, 1, 0);
        sides[0].Contracts[1] = MakeContract(/*E*/ 1, 5, /*C*/ 4, 1, 0);
        sides[1].Score = -100;
        sides[1].Number = 2;
        sides[1].Contracts[0] = MakeContract(/*EW*/ 5, 4, /*H*/ 2, 1, 0);
        sides[1].Contracts[1] = MakeContract(/*E*/ 1, 5, /*C*/ 4, 1, 0);

        Assert.Equal("Par: EW 4Hx, 5Cx -1 -100", DdTableForDealLib.FormatParLine(sides));
    }

    [Fact]
    public void PassedOut()
    {
        var sides = new ParResultsMaster[2];
        sides[0].Score = 0;
        sides[0].Number = 1;
        sides[1].Score = 0;
        sides[1].Number = 1;

        Assert.Equal("Par: 0", DdTableForDealLib.FormatParLine(sides));
    }

    [Fact]
    public void ReturnsNullWhenNoContractsDespiteScores()
    {
        var sides = new ParResultsMaster[2];
        sides[0].Score = -100;
        sides[0].Number = 0;
        sides[1].Score = 100;
        sides[1].Number = 0;

        Assert.Null(DdTableForDealLib.FormatParLine(sides));
    }
}

public class FormatTableTests
{
    [Fact]
    public void MatchesCppColumnOrder()
    {
        // strain rows: NT=4, then S/H/D/C = 0..3; columns North/South/East/West = 0,2,1,3
        var table = new DdTableResults();
        for (int strain = 0; strain < 5; strain++)
            for (int hand = 0; hand < 4; hand++)
                table.ResultsTable[strain, hand] = strain * 10 + hand;

        var text = DdTableForDealLib.FormatTable(table);
        var lines = text.Split('\n', StringSplitOptions.RemoveEmptyEntries);

        Assert.Equal(6, lines.Length);
        Assert.Equal("      North South East  West ", lines[0]);
        Assert.Equal("   NT    40    42    41    43", lines[1]);
        Assert.Equal("    S     0     2     1     3", lines[2]);
        Assert.Equal("    H    10    12    11    13", lines[3]);
        Assert.Equal("    D    20    22    21    23", lines[4]);
        Assert.Equal("    C    30    32    31    33", lines[5]);
    }
}

public class FormatPbnHandTests
{
    [Fact]
    public void EndsWithBlankLineAfterDiagram()
    {
        const string deal =
            "N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 "
            + "5.A95432.7632.K6 AKJ9842.K.T8.J93";

        string text = DdTableForDealLib.FormatPbnHand("dd_table_for_deal:\n", deal);

        Assert.EndsWith("\n\n", text);
    }
}
