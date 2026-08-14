using System.Text;
using DDS_Core;

namespace DdTableForDeal;

internal static class Program
{
    private static int Main(string[] args)
    {
        // ParseCli expects argv[0] = program name.
        var argv = new string[args.Length + 1];
        argv[0] = Environment.GetCommandLineArgs()[0];
        args.CopyTo(argv, 1);

        DdTableForDealLib.CliOptions? parsed;
        try
        {
            parsed = DdTableForDealLib.ParseCli(argv, stdinIsTty: !Console.IsInputRedirected);
        }
        catch (ArgumentException ex)
        {
            Console.Error.WriteLine(ex.Message);
            PrintUsage(Path.GetFileName(argv[0]));
            return 1;
        }

        if (parsed is null)
        {
            PrintUsage(Path.GetFileName(argv[0]));
            return 0;
        }

        IReadOnlyList<string> deals;
        try
        {
            deals = DdTableForDealLib.UniqueDeals(LoadDeals(parsed.Value.DealArg));
        }
        catch (Exception ex) when (ex is ArgumentException or IOException or InvalidOperationException)
        {
            Console.Error.WriteLine(ex.Message);
            return 1;
        }

        deals = DdTableForDealLib.ApplyDealLimit(deals, parsed.Value.Limit);

        if (deals.Any(d => d.Length >= DdTableForDealLib.PbnDealMax))
        {
            Console.Error.WriteLine(
                $"PBN deal too long (max {DdTableForDealLib.PbnDealMax - 1} characters)");
            return 1;
        }

        try
        {
            using var ctx = new SolverContext();
            var dds = new DDS();
            for (int i = 0; i < deals.Count; i++)
            {
                if (!ProcessDeal(ctx, dds, deals[i], i + 1, deals.Count, parsed.Value.Vulnerable))
                    return 1;
            }
        }
        catch (Exception ex) when (ex is DllNotFoundException or EntryPointNotFoundException
                                       or BadImageFormatException)
        {
            Console.Error.WriteLine(
                "Failed to load native DDS library. Build //jni:dds_shared and set "
                + "DDS_LIBRARY_PATH to the full path of libdds.dylib / libdds.so / dds.dll.");
            Console.Error.WriteLine(ex.Message);
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
        int vulnerable)
    {
        var tableDeal = new DdTableDealPBN { Cards = deal };

        try
        {
            ctx.CalcDdTable(tableDeal, out DdTableResults table);
            dds.ParAll(in table, vulnerable, out ParResultsMasters sidesMasters);

            string title = dealCount == 1
                ? "dd_table_for_deal:\n"
                : $"Deal {dealNo}:\n";

            Console.Write(DdTableForDealLib.FormatPbnHand(title, deal));
            Console.WriteLine(DdTableForDealLib.FormatTable(table));
            Console.WriteLine();

            var sides = new ParResultsMaster[] { sidesMasters[0], sidesMasters[1] };
            string? parLine = DdTableForDealLib.FormatParLine(sides);
            if (parLine is not null)
            {
                Console.WriteLine(parLine);
            }
            else
            {
                dds.Par(in table, vulnerable, out ParResults par);
                Console.Write(DdTableForDealLib.FormatParVerbose(par));
            }

            if (dealCount > 1)
                Console.WriteLine();
            return true;
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"DDS error: {ex.Message}");
            return false;
        }
    }

    private static IReadOnlyList<string> LoadDeals(string arg)
    {
        if (arg == "-")
        {
            string? text = ReadPbnStream(Console.OpenStandardInput());
            if (text is null)
                throw new InvalidOperationException("Cannot read PBN from stdin");
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
            text = ReadPbnStream(stream);
            return text is not null;
        }
        catch (IOException)
        {
            return false;
        }
    }

    private static string? ReadPbnStream(Stream stream)
    {
        using var reader = new StreamReader(stream, Encoding.UTF8, detectEncodingFromByteOrderMarks: true, leaveOpen: true);
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
                Console.Error.WriteLine(
                    $"PBN input too large (max {DdTableForDealLib.PbnFileMax} characters)");
                return null;
            }
        }
        return sb.ToString();
    }

    private static void PrintUsage(string prog)
    {
        Console.Error.Write(
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
