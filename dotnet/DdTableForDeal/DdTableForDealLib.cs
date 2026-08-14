using System.Text;
using System.Text.RegularExpressions;
using DDS_Core;

namespace DdTableForDeal;

/// <summary>
/// Pure helpers for the .NET <c>dd_table_for_deal</c> CLI — counterpart to
/// <c>examples/dd_table_for_deal_lib.cpp</c> / <c>python/examples/dd_table_for_deal.py</c>.
/// </summary>
public static partial class DdTableForDealLib
{
    public const int PbnFileMax = 16 * 1024 * 1024;
    public const int PbnDealMax = 80;

    // Matches dll.h contractType.denom: 0=NT, 1=S, 2=H, 3=D, 4=C.
    private static readonly char[] DenomChars = ['N', 'S', 'H', 'D', 'C'];
    private static readonly string[] SeatNames = ["N", "E", "S", "W", "NS", "EW"];

    private static readonly (string Label, int Strain)[] StrainRows =
    [
        ("NT", 4),
        ("S", 0),
        ("H", 1),
        ("D", 2),
        ("C", 3),
    ];

    // North, South, East, West — matches examples/hands.cpp print_table.
    private static readonly int[] HandColumns = [0, 2, 1, 3];

    private static readonly int[] BitMapRank =
    [
        0x0000, 0x0000, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020,
        0x0040, 0x0080, 0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000,
    ];

    private const string CardRankChars = "xx23456789TJQKA-";
    private const int DdsFullLine = 80;
    private const int DdsHandOffset = 12;
    private const int DdsHandLines = 12;

    public readonly record struct CliOptions(string DealArg, int Vulnerable, uint? Limit);

    [GeneratedRegex(@"\[Deal\s*""([^""]*)""", RegexOptions.IgnoreCase)]
    private static partial Regex DealTagRegex();

    public static int? ParseVulnerable(string text)
    {
        return text.ToLowerInvariant() switch
        {
            "none" or "0" => 0,
            "both" or "1" => 1,
            "ns" or "2" => 2,
            "ew" or "3" => 3,
            _ => null,
        };
    }

    public static uint? ParseLimit(string text)
    {
        if (string.IsNullOrEmpty(text))
            return null;

        uint value = 0;
        foreach (char ch in text)
        {
            if (ch is < '0' or > '9')
                return null;
            uint digit = (uint)(ch - '0');
            if (value > (uint.MaxValue - digit) / 10)
                return null;
            value = value * 10 + digit;
        }

        return value == 0 ? null : value;
    }

    public static IReadOnlyList<string> ApplyDealLimit(
        IReadOnlyList<string> deals, uint? limit)
    {
        if (limit is null || limit.Value >= deals.Count)
            return deals is List<string> list ? list : deals.ToList();
        return deals.Take((int)limit.Value).ToList();
    }

    public static IReadOnlyList<string> ExtractDealTags(string text)
    {
        var deals = new List<string>();
        foreach (Match match in DealTagRegex().Matches(text))
            deals.Add(match.Groups[1].Value);
        return deals;
    }

    public static IReadOnlyList<string> UniqueDeals(IEnumerable<string> deals)
    {
        var unique = new List<string>();
        var seen = new HashSet<string>();
        foreach (string deal in deals)
        {
            if (seen.Add(deal))
                unique.Add(deal);
        }
        return unique;
    }

    public static bool LooksLikePath(string arg)
    {
        if (arg.Contains('/') || arg.Contains('\\'))
            return true;
        return arg.EndsWith(".pbn", StringComparison.OrdinalIgnoreCase)
            || arg.EndsWith(".txt", StringComparison.OrdinalIgnoreCase);
    }

    /// <summary>
    /// Parse CLI args (argv[0] is the program name). Returns null for help.
    /// When <paramref name="stdinIsTty"/> is false and no deal arg is given, uses "-".
    /// </summary>
    public static CliOptions? ParseCli(IReadOnlyList<string> argv, bool stdinIsTty = true)
    {
        string? deal = null;
        int vulnerable = 0;
        uint? limit = null;

        for (int i = 1; i < argv.Count; i++)
        {
            string arg = argv[i];
            if (arg is "-h" or "--help")
                return null;

            if (arg == "--vul")
            {
                if (i + 1 >= argv.Count)
                    throw new ArgumentException(
                        "--vul requires a value (none|both|ns|ew or 0|1|2|3)");
                vulnerable = ParseVulnerable(argv[++i])
                    ?? throw new ArgumentException(
                        "Invalid --vul value (use none|both|ns|ew or 0|1|2|3)");
                continue;
            }

            if (arg == "--limit")
            {
                if (i + 1 >= argv.Count)
                    throw new ArgumentException("--limit requires a positive integer");
                limit = ParseLimit(argv[++i])
                    ?? throw new ArgumentException(
                        "Invalid --limit value (use a positive integer)");
                continue;
            }

            if (arg.StartsWith('-') && arg != "-")
                throw new ArgumentException($"Unknown option: {arg}");

            if (deal is not null)
                throw new ArgumentException("Only one deal argument is allowed");

            deal = arg;
        }

        if (deal is null)
        {
            if (!stdinIsTty)
                deal = "-";
            else
                throw new ArgumentException("missing deal argument");
        }

        return new CliOptions(deal, vulnerable, limit);
    }

    public static string? FormatParLine(ReadOnlySpan<ParResultsMaster> sides)
    {
        if (sides.Length < 2)
            return null;

        if (sides[0].Score == 0 && sides[1].Score == 0)
            return "Par: 0";

        if (sides[0].Number <= 0 && sides[1].Number <= 0)
            return null;

        ContractType first = sides[0].Number > 0
            ? sides[0].Contracts[0]
            : sides[1].Contracts[0];
        int side = first.Seats is 4 or 0 or 2 ? 0 : 1;
        ParResultsMaster chosen = sides[side];
        if (chosen.Number <= 0)
            return null;

        var body = new StringBuilder();
        for (int i = 0; i < chosen.Number; i++)
        {
            string? piece = FormatContract(chosen.Contracts[i], includeSeats: i == 0);
            if (piece is null)
                return null;
            if (i > 0)
                body.Append(", ");
            body.Append(piece);
        }

        ContractType contract = chosen.Contracts[0];
        string result = contract.UnderTricks > 0
            ? $"-{contract.UnderTricks}"
            : contract.OverTricks > 0
                ? $"+{contract.OverTricks}"
                : "=";

        return $"Par: {body} {result} {chosen.Score}";
    }

    public static string FormatTable(in DdTableResults table)
    {
        var sb = new StringBuilder();
        sb.AppendFormat("{0,5} {1,-5} {2,-5} {3,-5} {4,-5}\n",
            "", "North", "South", "East", "West");

        foreach ((string label, int strain) in StrainRows)
        {
            sb.AppendFormat("{0,5} {1,5} {2,5} {3,5} {4,5}\n",
                label,
                table.ResultsTable[strain, HandColumns[0]],
                table.ResultsTable[strain, HandColumns[1]],
                table.ResultsTable[strain, HandColumns[2]],
                table.ResultsTable[strain, HandColumns[3]]);
        }

        return sb.ToString().TrimEnd('\n');
    }

    public static string FormatPbnHand(string title, string pbnDeal)
    {
        uint[,] remainCards = ConvertPbn(pbnDeal);
        char[][] text = new char[DdsHandLines][];
        int[] rowEnds = new int[DdsHandLines];
        for (int i = 0; i < DdsHandLines; i++)
        {
            text[i] = new char[DdsFullLine];
            Array.Fill(text[i], ' ');
            rowEnds[i] = DdsFullLine;
        }

        for (int h = 0; h < 4; h++)
        {
            int offset, line;
            switch (h)
            {
                case 0: offset = DdsHandOffset; line = 0; break;
                case 1: offset = 2 * DdsHandOffset; line = 4; break;
                case 2: offset = DdsHandOffset; line = 8; break;
                default: offset = 0; line = 4; break;
            }

            for (int s = 0; s < 4; s++)
            {
                int row = line + s;
                int c = offset;
                for (int r = 14; r >= 2; r--)
                {
                    if (((remainCards[h, s] >> 2) & BitMapRank[r]) != 0)
                        text[row][c++] = CardRankChars[r];
                }
                if (c == offset)
                    text[row][c++] = '-';
                if (h != 3)
                    rowEnds[row] = c;
            }
        }

        var sb = new StringBuilder();
        sb.Append(title);
        int dashLen = Math.Max(0, title.Length - 1);
        sb.Append('-', dashLen).Append('\n');
        for (int i = 0; i < DdsHandLines; i++)
            sb.Append(text[i], 0, rowEnds[i]).Append('\n');
        sb.Append('\n'); // blank line after the diagram (matches C++/Python)
        return sb.ToString();
    }

    public static string FormatParVerbose(in ParResults par)
    {
        return
            $"NS score: {par.ParScores[0]}\n" +
            $"EW score: {par.ParScores[1]}\n" +
            $"NS list : {par.ParContractStrings[0]}\n" +
            $"EW list : {par.ParContractStrings[1]}\n";
    }

    public static uint[,] ConvertPbn(string pbnDeal)
    {
        var remain = new uint[4, 4];
        int bp = 0;
        while (bp < 3 && bp < pbnDeal.Length
               && pbnDeal[bp] is not ('N' or 'W' or 'E' or 'S'
                                      or 'n' or 'w' or 'e' or 's'))
            bp++;
        if (bp >= 3 || bp >= pbnDeal.Length)
            return remain;

        int first = char.ToUpperInvariant(pbnDeal[bp]) switch
        {
            'N' => 0,
            'E' => 1,
            'S' => 2,
            _ => 3,
        };
        bp += 2;
        int handRelFirst = 0;
        int suitInHand = 0;

        while (bp < 80 && bp < pbnDeal.Length)
        {
            char ch = pbnDeal[bp];
            int card = IsCard(ch);
            if (card != 0)
            {
                int hand = first switch
                {
                    0 => handRelFirst,
                    1 => handRelFirst == 0 ? 1 : handRelFirst == 3 ? 0 : handRelFirst + 1,
                    2 => handRelFirst == 0 ? 2 : handRelFirst == 1 ? 3 : handRelFirst - 2,
                    _ => handRelFirst == 0 ? 3 : handRelFirst - 1,
                };
                remain[hand, suitInHand] |= (uint)(BitMapRank[card] << 2);
            }
            else if (ch == '.')
            {
                suitInHand++;
            }
            else if (ch == ' ')
            {
                handRelFirst++;
                suitInHand = 0;
            }
            bp++;
        }
        return remain;
    }

    private static int IsCard(char ch)
    {
        return char.ToUpperInvariant(ch) switch
        {
            '2' => 2, '3' => 3, '4' => 4, '5' => 5, '6' => 6,
            '7' => 7, '8' => 8, '9' => 9, 'T' => 10, 'J' => 11,
            'Q' => 12, 'K' => 13, 'A' => 14,
            _ => 0,
        };
    }

    private static string? FormatContract(in ContractType contract, bool includeSeats)
    {
        if (contract.Denomination is < 0 or > 4)
            return null;
        if (contract.Seats is < 0 or > 5)
            return null;

        char denom = DenomChars[contract.Denomination];
        bool doubled = contract.UnderTricks > 0;
        string body = doubled
            ? $"{contract.Level}{denom}x"
            : $"{contract.Level}{denom}";
        return includeSeats ? $"{SeatNames[contract.Seats]} {body}" : body;
    }
}
