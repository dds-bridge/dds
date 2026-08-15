using System.Text;
using DDS_Core;

namespace DdTableForDeal;

/// <summary>
/// Testable entry point for the <c>dd_table_for_deal</c> CLI.
/// </summary>
public static class DdTableForDealApp
{
    /// <param name="argv">Full argv including program name at index 0.</param>
    public static int Run(
        IReadOnlyList<string> argv,
        TextWriter stdout,
        TextWriter stderr,
        bool stdinIsTty = true,
        TextReader? stdin = null)
    {
        DdTableForDealLib.CliOptions? parsed;
        try
        {
            parsed = DdTableForDealLib.ParseCli(argv, stdinIsTty);
        }
        catch (ArgumentException ex)
        {
            stderr.WriteLine(ex.Message);
            PrintUsage(stderr, Path.GetFileName(argv[0]));
            return 1;
        }

        if (parsed is null)
        {
            PrintUsage(stderr, Path.GetFileName(argv[0]));
            return 0;
        }

        IReadOnlyList<string> deals;
        try
        {
            deals = DdTableForDealLib.UniqueDeals(
                LoadDeals(parsed.Value.DealArg, stdin));
        }
        catch (Exception ex) when (ex is ArgumentException or IOException
                                       or InvalidOperationException
                                       or UnauthorizedAccessException)
        {
            stderr.WriteLine(ex.Message);
            return 1;
        }

        deals = DdTableForDealLib.ApplyDealLimit(deals, parsed.Value.Limit);

        if (deals.Any(d => d.Length >= DdTableForDealLib.PbnDealMax))
        {
            stderr.WriteLine(
                $"PBN deal too long (max {DdTableForDealLib.PbnDealMax - 1} characters)");
            return 1;
        }

        try
        {
            using var ctx = new SolverContext();
            var dds = new DDS();
            for (int i = 0; i < deals.Count; i++)
            {
                if (!ProcessDeal(
                        ctx, dds, deals[i], i + 1, deals.Count,
                        parsed.Value.Vulnerable, stdout, stderr))
                {
                    return 1;
                }
            }
        }
        catch (Exception ex) when (ex is DllNotFoundException or EntryPointNotFoundException
                                       or BadImageFormatException)
        {
            stderr.WriteLine(
                "Failed to load native DDS library. Build //jni:dds_shared and set "
                + "DDS_LIBRARY_PATH to the full path of libdds.dylib / libdds.so / dds.dll.");
            stderr.WriteLine(ex.Message);
            return 1;
        }

        return 0;
    }

    private static bool ProcessDeal(
        SolverContext ctx,
        DDS dds,
        string deal,
        int dealNo,
        int dealCount,
        int vulnerable,
        TextWriter stdout,
        TextWriter stderr)
    {
        var tableDeal = new DdTableDealPBN { Cards = deal };

        try
        {
            ctx.CalcDdTable(tableDeal, out DdTableResults table);
            dds.ParAll(in table, vulnerable, out ParResultsMasters sidesMasters);

            string title = dealCount == 1
                ? "dd_table_for_deal:\n"
                : $"Deal {dealNo}:\n";

            stdout.Write(DdTableForDealLib.FormatPbnHand(title, deal));
            stdout.WriteLine(DdTableForDealLib.FormatTable(table));
            stdout.WriteLine();

            var sides = new ParResultsMaster[] { sidesMasters[0], sidesMasters[1] };
            string? parLine = DdTableForDealLib.FormatParLine(sides);
            if (parLine is not null)
            {
                stdout.WriteLine(parLine);
            }
            else
            {
                dds.Par(in table, vulnerable, out ParResults par);
                stdout.Write(DdTableForDealLib.FormatParVerbose(par));
            }

            if (dealCount > 1)
                stdout.WriteLine();
            return true;
        }
        catch (Exception ex)
        {
            stderr.WriteLine($"DDS error: {ex.Message}");
            return false;
        }
    }

    private static IReadOnlyList<string> LoadDeals(
        string arg, TextReader? stdin)
    {
        if (arg == "-")
        {
            string text = ReadPbnStream(stdin ?? Console.In);
            var deals = DdTableForDealLib.ExtractDealTags(text);
            if (deals.Count == 0)
                throw new InvalidOperationException("No [Deal \"...\"] tag found in stdin");
            return deals;
        }

        string? fileText = ReadPbnFileWorkspaceRelative(arg);
        if (fileText is not null)
        {
            var deals = DdTableForDealLib.ExtractDealTags(fileText);
            if (deals.Count == 0)
                throw new InvalidOperationException($"No [Deal \"...\"] tag found in {arg}");
            return deals;
        }

        if (DdTableForDealLib.LooksLikePath(arg))
            throw new FileNotFoundException($"Cannot read file: {arg}", arg);

        if (arg.Length >= DdTableForDealLib.PbnDealMax)
            throw new ArgumentException(
                $"PBN deal too long (max {DdTableForDealLib.PbnDealMax - 1} characters)");

        return [arg];
    }

    private static string? ReadPbnFileWorkspaceRelative(string path)
    {
        if (TryReadPbnFile(path, out string? text))
            return text;

        string? workspace = Environment.GetEnvironmentVariable("BUILD_WORKSPACE_DIRECTORY");
        if (workspace is not null
            && TryReadPbnFile(Path.Combine(workspace, path), out text))
        {
            return text;
        }

        return null;
    }

    private static bool TryReadPbnFile(string path, out string? text)
    {
        text = null;
        try
        {
            using var stream = File.OpenRead(path);
            using var reader = new StreamReader(stream, Encoding.UTF8, detectEncodingFromByteOrderMarks: true);
            text = ReadPbnStream(reader);
            return true;
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException)
        {
            return false;
        }
    }

    private static string ReadPbnStream(TextReader reader)
    {
        var sb = new StringBuilder();
        char[] buffer = new char[4096];
        while (true)
        {
            int n = reader.Read(buffer, 0, buffer.Length);
            if (n <= 0)
                break;
            sb.Append(buffer, 0, n);
            if (sb.Length > DdTableForDealLib.PbnFileMax)
            {
                throw new InvalidOperationException(
                    $"PBN input too large (max {DdTableForDealLib.PbnFileMax} characters)");
            }
        }
        return sb.ToString();
    }

    private static void PrintUsage(TextWriter stderr, string prog)
    {
        stderr.Write(
            $"Usage: {prog} [--vul none|both|ns|ew|0|1|2|3] [--limit N] "
            + $"<pbn_deal_or_file>\n"
            + $"       {prog} -h | --help\n"
            + "\n"
            + "Calculate double-dummy tricks and par for all strains and leads.\n"
            + "\n"
            + "Arguments:\n"
            + "  <pbn_deal_or_file>  DDS PBN deal string, or path to a .pbn file\n"
            + "  --vul              Vulnerability: none|both|ns|ew or 0|1|2|3"
            + " (default: none)\n"
            + "  --limit            Solve only the first N unique deals\n"
            + "\n"
            + "If stdin is not a terminal, PBN is read from stdin (all [Deal \"...\"] tags).\n"
            + "\n"
            + "Examples:\n"
            + $"  {prog} \"N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 "
            + "5.A95432.7632.K6 AKJ9842.K.T8.J93\"\n"
            + $"  {prog} --vul ns hands/example.pbn\n"
            + $"  {prog} --limit 3 hands/multi_board.pbn\n"
            + $"  {prog} < hands/example.pbn\n");
    }
}
