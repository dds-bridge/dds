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
}
