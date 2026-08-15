namespace DdTableForDeal.Tests;

/// <summary>
/// Error-path coverage for CLI PBN loading (Copilot review on PR #321).
/// </summary>
public class PbnLoadErrorTests
{
    [Fact]
    public void Run_OversizedStdin_ReportsSizeNotGenericStdinFailure()
    {
        string huge = new('x', DdTableForDealLib.PbnFileMax + 1);
        var stdout = new StringWriter();
        var stderr = new StringWriter();

        int rc = DdTableForDealApp.Run(
            ["dd_table_for_deal", "-"],
            stdout,
            stderr,
            stdinIsTty: false,
            stdin: new StringReader(huge));

        Assert.Equal(1, rc);
        Assert.Contains("PBN input too large", stderr.ToString());
        Assert.DoesNotContain("Cannot read PBN from stdin", stderr.ToString());
    }

    [Fact]
    public void Run_OversizedPbnFile_ReportsSizeNotCannotReadFile()
    {
        string path = Path.Combine(Path.GetTempPath(), $"dds_oversized_{Guid.NewGuid():N}.pbn");
        File.WriteAllText(path, new string('x', DdTableForDealLib.PbnFileMax + 1));
        try
        {
            var stdout = new StringWriter();
            var stderr = new StringWriter();

            int rc = DdTableForDealApp.Run(
                ["dd_table_for_deal", path],
                stdout,
                stderr);

            Assert.Equal(1, rc);
            Assert.Contains("PBN input too large", stderr.ToString());
            Assert.DoesNotContain("Cannot read file:", stderr.ToString());
        }
        finally
        {
            File.Delete(path);
        }
    }

    [Fact]
    public void Run_UnreadablePbnFile_ReportsCannotReadFile()
    {
        // UnauthorizedAccessException is hard to force portably on Windows CI;
        // cover the Unix permission-denied path which hits the same catch.
        if (OperatingSystem.IsWindows())
            return;

        string path = Path.Combine(Path.GetTempPath(), $"dds_denied_{Guid.NewGuid():N}.pbn");
        File.WriteAllText(path, "[Deal \"N:..\"]\n");
        File.SetUnixFileMode(path, UnixFileMode.None);
        try
        {
            var stdout = new StringWriter();
            var stderr = new StringWriter();

            int rc = DdTableForDealApp.Run(
                ["dd_table_for_deal", path],
                stdout,
                stderr);

            Assert.Equal(1, rc);
            Assert.Contains("Cannot read file:", stderr.ToString());
        }
        finally
        {
            File.SetUnixFileMode(path, UnixFileMode.UserRead | UnixFileMode.UserWrite);
            File.Delete(path);
        }
    }

    [Fact]
    public void Run_InvalidPathCharacters_ReportsCannotReadFile()
    {
        // Null in the path throws ArgumentException from File.OpenRead on every OS.
        string path = "bad\0name.pbn";
        Assert.True(DdTableForDealLib.LooksLikePath(path));

        var stdout = new StringWriter();
        var stderr = new StringWriter();

        int rc = DdTableForDealApp.Run(
            ["dd_table_for_deal", path],
            stdout,
            stderr);

        Assert.Equal(1, rc);
        Assert.Contains("Cannot read file:", stderr.ToString());
    }
}
