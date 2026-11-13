using DDS_Core.Helpers;

namespace DDS_Core.Tests;

/// <summary>
/// <see cref="SolveBoardResult"/> previously stopped at ThreadMissing (-104),
/// with no named members for the CalcAllTables*/Par* family of return codes
/// that DDS.CalcAllTables/Par/CalcPar/etc. (DDS.cs) actually return.
/// </summary>
public class SolveBoardResultTests
{
    [Theory]
    [InlineData(SolveBoardResult.NoSuit, -201)]
    [InlineData(SolveBoardResult.TooManyTables, -202)]
    [InlineData(SolveBoardResult.ChunkSize, -301)]
    [InlineData(SolveBoardResult.ParTableFault, -401)]
    public void SolveBoardResult_HasValue(SolveBoardResult result, int expected)
    {
        Assert.Equal(expected, (int)result);
    }

    /// <summary>
    /// The four new members above went straight from <see cref="SolveBoardResult"/>
    /// into the PR without a matching case in
    /// <see cref="SolveBoardResultExtensions.GetRcErrorMessage"/>, so each fell through
    /// to its "Unknown error" default instead of the message DDS actually documents for it.
    /// </summary>
    [Theory]
    [InlineData(SolveBoardResult.NoSuit, "Denomination filter vector has no entries")]
    [InlineData(SolveBoardResult.TooManyTables, "Too many DD tables requested")]
    [InlineData(SolveBoardResult.ChunkSize, "Chunk size is less than 1")]
    [InlineData(SolveBoardResult.ParTableFault, "Missing double dummy table, or an entry outside the range 0 to 13")]
    public void SolveBoardResult_HasErrorMessage(SolveBoardResult result, string expected)
    {
        Assert.Equal(expected, result.GetRcErrorMessage());
    }
}
