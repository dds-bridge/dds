namespace DdTableForDeal.Tests;

/// <summary>
/// Distinguishes solver failures from unexpected runtime errors in ProcessDeal.
/// </summary>
public class ProcessDealErrorTests
{
    [Fact]
    public void FormatProcessDealFailure_InvalidOperation_IsDdsError()
    {
        string message = DdTableForDealApp.FormatProcessDealFailure(
            new InvalidOperationException("CalcDdTable failed with code -1: PBN string error"));

        Assert.Equal(
            "DDS error: CalcDdTable failed with code -1: PBN string error",
            message);
    }

    [Fact]
    public void FormatProcessDealFailure_OtherException_IsUnexpectedError()
    {
        string message = DdTableForDealApp.FormatProcessDealFailure(
            new IOException("simulated stdout failure"));

        Assert.Equal("Unexpected error: simulated stdout failure", message);
    }
}
