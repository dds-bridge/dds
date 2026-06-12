using System.Diagnostics;
using DDS_Core;

namespace DDS_Core_Demo;

internal class Program
{
    private static string[] contracts  = ["spades","hearts", "diamonds", "clubs", "no trump" ];
    private static string[] c          = ["S","H", "D", "C", "NT" ];
    private static int[,]   tricks     = new int[5, 4];
    private static bool     isPerformanceTest ;
    private static int      iterations = 100;

    static void Main(string[] args)
    {
        var dds = new DDS();
        var tst =TestData.deals; // Only to initialize TestData - actually not needed!

        // All samples display the number of tricks for each contract
        // and each declarer even though most of the dds methods returns
        // the number of tricks for the hand at lead.
        //
        // PERFORMANCE TEST: Measure AnalyseAllPlaysBin P/Invoke performance
        // Run in Release mode for accurate results
        isPerformanceTest = args.Length >  0 && args[0] == "benchmark";

        if (isPerformanceTest)
        {
            BenchmarkAnalyseAllPlaysBin(dds);
            Console.WriteLine($"Press any key to continue...");
            Console.ReadKey();
            return;
        }

        int vulnability =2; // 0=none, 1=both, 2=NS, 3=EW
        int dealer      = 1;    // 0=N, 1=E, 2=S, 3=W

        //
        doSolveBoard(dds, TestData.deals[0]);
        doSolveBoard(dds, TestData.dealsPBN[0]);

        doSolveAllBoards(dds, TestData.boards);
        doSolveAllBoards(dds, TestData.boardsPBN);

        // Obsolete methods - not recommended for use as they are not optimized and will be removed in future versions
        //doSolveAllChunks(dds, TestData.boards, 10);
        //doSolveAllChunks(dds, TestData.boardsPBN, 10);        
        //doSolveAllChunksPBN(dds, TestData.boardsPBN, 10);
        ////
        doCalcDdTable(dds, TestData.ddTableDeal);
        doCalcDdTable(dds, TestData.ddTableDealPBN);

        doCalcAllTables(dds, TestData.ddTableDeals);
        doCalcAllTables(dds, TestData.ddTableDealsPBN);
        //
        doCalcPar(dds, TestData.ddTableDeal, vulnability);
        doCalcPar(dds, TestData.ddTableDealPBN, vulnability);
        doPar(dds, TestData.ddTableResults, vulnability);

        doParDealer(dds, TestData.ddTableResults, dealer, vulnability);
        doParDealerBothSides(dds, TestData.ddTableResults, dealer, vulnability);

        doParAll(dds, TestData.ddTableResults, vulnability);
        doParSide(dds, TestData.ddTableResults, vulnability);

        doConvertToTextFormat(dds, TestData.parResultsMaster);
        doConvertToTextFormat(dds, TestData.parResultsMasters);

        doAnalysePlay(dds, TestData.deals[0], TestData.parResultsMasters, TestData.playTraceBin);
        doAnalysePlay(dds, TestData.dealsPBN[0], TestData.parResultsMasters, TestData.playTracePBN);
        //
        doAnalyseAllPlays(dds, TestData.boards, TestData.playTracesBin);
        doAnalyseAllPlays(dds, TestData.boardsPBN, TestData.playTracesPBN);
        //
        doGetDDSInfo(dds);
        doErrorMessage(dds, -14);

        #region Version 3.0.0 samples
            var cfg = new SolverConfig()
                      {
                          TTKind          = TTKind.Large
                        , DefaultMemoryMB = 256
                        , MaximumMemoryMB = 1024
                      };

            using (var ctx = new SolverContext(cfg))
            {
                ctx?.ConfigureTT(TTKind.Small, 256, 1024);

                doSolveBoardV3(dds, ctx, TestData.deals[0]);
                doCalcDdTableV3(dds, ctx, TestData.ddTableDeal);
                doCalcDdTableV3(dds, ctx, TestData.ddTableDealPBN);
                doCalcParV3(dds, ctx, TestData.ddTableDeal, vulnability);
                ctx.LogAppend("Completed V3.0.0 samples");
                //ctx.LogClear();
            }
        #endregion

        Console.WriteLine($"Press any key to continue...");
        Console.ReadKey();
        return;
    }

    private static void doSolveBoard(DDS dds, Deal deal)
    {
        Console.WriteLine($"SolveBoard");

        tricks = new int[5, 4];

        for (deal.Trump = 0; deal.Trump <  5; deal.Trump++)
            for (deal.First = 0; deal.First <  4; deal.First++)
            {
                var decl =(deal.First + 3) & 3;
                var rc   = dds.SolveBoard(deal, -1, 1, 0, out FutureTricks fut);

                // record the number of tricks for declarer
                tricks[deal.Trump, decl] = 13 - fut.Score[0];
                dds.FreeMemory();
            }

        DisplayTricks();
    }

    private static void doSolveBoardV3(DDS dds, SolverContext ctx, Deal deal)
    {
        Console.WriteLine($"SolveBoard v3");

        tricks = new int[5, 4];

        for (deal.Trump = 0; deal.Trump <  5; deal.Trump++)
            for (deal.First = 0; deal.First <  4; deal.First++)
            {
                var decl =(deal.First + 3) & 3;
                var rc   = ctx.SolveBoard(deal, -1, 1, 0, out FutureTricks fut);

                // record the number of tricks for declarer
                tricks[deal.Trump, decl] = 13 - fut.Score[0];
                ctx.ResetForSolve();
            }


        DisplayTricks();
    }

    private static void doSolveBoard(DDS dds, DealPBN deal)
    {
        // SolveBoard: Loop through all possible contracts and first players
        Console.WriteLine($"SolveBoard PBN");
        tricks = new int[5, 4];

        for (deal.Trump = 0; deal.Trump <  5; deal.Trump++)

            for (deal.First = 0; deal.First <  4; deal.First++)
            {
                var decl =(deal.First + 3) & 3;
                var rc   = dds.SolveBoard(deal, -1, 1, 0, out FutureTricks fut);

                // record the number of tricks for declarer
                tricks[deal.Trump, decl] = 13 - fut.Score[0];

                dds.FreeMemory();
            }

        DisplayTricks();
    }

    private static void doSolveAllBoards(DDS dds, BoardsPBN boards)
    {
        Console.WriteLine($"SolveAllBoards PBN");
        tricks = new int[5, 4];

        for (boards.Deals[0].Trump = 0; boards.Deals[0].Trump <  5; boards.Deals[0].Trump++)
            for (boards.Deals[0].First = 0; boards.Deals[0].First <  4; boards.Deals[0].First++)
            {
                var decl =(boards.Deals[0].First + 3) & 3;
                var rc   = dds.SolveAllBoards(boards, out SolvedBoards solved);

                // record the number of tricks for declarer
                tricks[boards.Deals[0].Trump, decl] = 13 - solved.Tricks[0].Score[0];

                dds.FreeMemory();
            }

        DisplayTricks();
    }

    private static void doSolveAllBoards(DDS dds, Boards boards)
    {
        Console.WriteLine($"SolveAllBoards");
        tricks = new int[5, 4];

        // Here we loop over all trump suits and dealers which is't the normal thing to do!
        // This is done as we want to display the full tricks table
        for (boards.Deals[0].Trump = 0; boards.Deals[0].Trump <  5; boards.Deals[0].Trump++)
            for (boards.Deals[0].First = 0; boards.Deals[0].First <  4; boards.Deals[0].First++)
            {
                var decl =(boards.Deals[0].First + 3) & 3;
                var rc   = dds.SolveAllBoards(boards, out SolvedBoards solved);

                // record the number of tricks for declarer
                tricks[boards.Deals[0].Trump, decl] = 13 - solved.Tricks[0].Score[0];

                dds.FreeMemory();
            }

        DisplayTricks();
    }

    //private static void doSolveAllChunks(DDS dds, BoardsPBN boards, int chunkSize)
    //{
    //    Console.WriteLine($"SolveAllChunks PBN");
    //    tricks = new int[5, 4];

    //    for (boards.Deals[0].Trump = 0; boards.Deals[0].Trump <  5; boards.Deals[0].Trump++)
    //        for (boards.Deals[0].First = 0; boards.Deals[0].First <  4; boards.Deals[0].First++)
    //        {
    //            var decl =(boards.Deals[0].First + 3) & 3;
    //            var rc   = dds.SolveAllChunks(boards, out SolvedBoards solved, chunkSize);

    //            // record the number of tricks for declarer
    //            tricks[boards.Deals[0].Trump, decl] = 13 - solved.Tricks[0].Score[0];

    //            dds.FreeMemory();
    //        }

    //    DisplayTricks();
    //}

    //private static void doSolveAllChunks(DDS dds, Boards boards, int chunkSize)
    //{
    //    Console.WriteLine($"SolveAllChunks");
    //    tricks = new int[5, 4];

    //    for (boards.Deals[0].Trump = 0; boards.Deals[0].Trump <  5; boards.Deals[0].Trump++)
    //        for (boards.Deals[0].First = 0; boards.Deals[0].First <  4; boards.Deals[0].First++)
    //        {
    //            var decl =(boards.Deals[0].First + 3) & 3;
    //            var rc   = dds.SolveAllChunksBin(boards, out SolvedBoards solved, chunkSize);

    //            // record the number of tricks for declarer
    //            tricks[boards.Deals[0].Trump, decl] = 13 - solved.Tricks[0].Score[0];

    //            dds.FreeMemory();
    //        }

    //    DisplayTricks();
    //}

    //private static void doSolveAllChunksPBN(DDS dds, BoardsPBN boards, int chunkSize)
    //{
    //    Console.WriteLine($"SolveAllChunks PBN");
    //    tricks = new int[5, 4];

    //    for (boards.Deals[0].Trump = 0; boards.Deals[0].Trump <  5; boards.Deals[0].Trump++)
    //        for (boards.Deals[0].First = 0; boards.Deals[0].First <  4; boards.Deals[0].First++)
    //        {
    //            var decl =(boards.Deals[0].First + 3) & 3;
    //            var rc   = dds.SolveAllChunksPBN(boards, out SolvedBoards solved, chunkSize);

    //            // record the number of tricks for declarer
    //            tricks[boards.Deals[0].Trump, decl] = 13 - solved.Tricks[0].Score[0];

    //            dds.FreeMemory();
    //        }

    //    DisplayTricks();
    //}
    private static void doCalcDdTable(DDS dds, DdTableDeal ddTableDeal)
    {
        Console.WriteLine($"CalcDdTable");
        tricks = new int[5, 4];

        var rc = dds.CalcDdTable(ddTableDeal, out DdTableResults results);

        for (var trump = 0; trump <  5; trump++)

            for (var first = 0; first <  4; first++)
            {
                var decl =(first + 3) & 3;

                // record the number of tricks for declarer
                tricks[trump, decl] = results.ResultsTable[trump, decl];
            }

        dds.FreeMemory();

        DisplayTricks();

        TestData.ddTableResults = results;
    }

    private static void doCalcDdTableV3(DDS dds, SolverContext ctx, DdTableDeal ddTableDeal)
    {
        Console.WriteLine($"CalcDdTable V3");
        tricks = new int[5, 4];

        var rc = ctx.CalcDdTable(ddTableDeal, out DdTableResults results);

        for (var trump = 0; trump <  5; trump++)

            for (var first = 0; first <  4; first++)
            {
                var decl =(first + 3) & 3;

                // record the number of tricks for declarer
                tricks[trump, decl] = results.ResultsTable[trump, decl];
            }

        ctx.ResetForSolve();
        DisplayTricks();
    }

    private static void doCalcDdTable(
                                       DDS dds, DdTableDealPBN ddTableDeal)
    {
        Console.WriteLine($"CalcDdTable PBN");
        tricks = new int[5, 4];

        var rc = dds.CalcDdTable(ddTableDeal, out DdTableResults results);

        for (var trump = 0; trump <  5; trump++)

            for (var first = 0; first <  4; first++)
            {
                var decl =(first + 3) & 3;

                // record the number of tricks for declarer
                tricks[trump, decl] = results.ResultsTable[trump, decl];
            }

        dds.FreeMemory();

        DisplayTricks();

        TestData.ddTableResults = results;
    }

    private static void doCalcDdTableV3(DDS dds, SolverContext ctx, DdTableDealPBN ddTableDeal)
    {
        Console.WriteLine($"CalcDdTable PBN V3");
        tricks = new int[5, 4];

        var rc = ctx.CalcDdTable(ddTableDeal, out DdTableResults results);

        for (var trump = 0; trump <  5; trump++)

            for (var first = 0; first <  4; first++)
            {
                var decl =(first + 3) & 3;

                // record the number of tricks for declarer
                tricks[trump, decl] = results.ResultsTable[trump, decl];
            }

        ctx.ResetForSolve();
        DisplayTricks();

        TestData.ddTableResults = results;
    }

    private static void doCalcAllTables(DDS dds, DdTableDeals ddTableDeals)
    {
        Console.WriteLine($"CalcAllTables");
        tricks                = new int[5, 4];
        intArray5 trumpFilter = new();

        var rc = dds.CalcAllTables( in ddTableDeals
                                  , 0
                                  , trumpFilter
                                  , out DdTablesResult results
                                  , out AllParResults presp);

        for (var trump = 0; trump <  5; trump++)

            for (var first = 0; first <  4; first++)
            {
                var decl =(first + 3) & 3;

                // record the number of tricks for declarer
                tricks[trump, decl] = results[0].ResultsTable[trump, decl];
            }

        dds.FreeMemory();

        DisplayTricks();
    }

    private static void doCalcAllTables(DDS dds, DdTableDealsPBN ddTableDeals)
    {
        Console.WriteLine($"CalcAllTables PBN");
        tricks                = new int[5, 4];
        intArray5 trumpFilter = new();

        var rc = dds.CalcAllTables( in ddTableDeals
                                  , 0
                                  , trumpFilter
                                  , out DdTablesResult results
                                  , out AllParResults presp);

        for (var trump = 0; trump <  5; trump++)

            for (var first = 0; first <  4; first++)
            {
                var decl =(first + 3) & 3;

                // record the number of tricks for declarer
                tricks[trump, decl] = results[0].ResultsTable[trump, decl];
            }

        dds.FreeMemory();

        DisplayTricks();
    }

    private static void doPar(DDS dds, DdTableResults tableResults, int vulnability)
    {
        Console.WriteLine($"Par");
        tricks = new int[5, 4];

        var rc = dds.Par( in tableResults
                        , out ParResults results
                        , vulnability);

        Console.WriteLine(results.ParContractStrings);
        Console.WriteLine(results.ParScores);

        dds.FreeMemory();
        Console.WriteLine();
    }

    private static void doCalcParV3(DDS dds, SolverContext ctx, DdTableDeal ddTableDeal, int vulnability)
    {
        Console.WriteLine($"CalcPar V3");
        tricks = new int[5, 4];

        var rc = ctx.CalcPar( in ddTableDeal
                            , vulnability
                            , out DdTableResults tResults
                            , out ParResults results                        );

        Console.WriteLine(results.ParContractStrings);
        Console.WriteLine(results.ParScores);

        for (var trump = 0; trump <  5; trump++)
            for (var first = 0; first <  4; first++)
            {
                var decl =(first + 3) & 3;

                // record the number of tricks for declarer
                tricks[trump, decl] = tResults.ResultsTable[trump, decl];
            }

        ctx.ResetForSolve();
        DisplayTricks();
    }

    private static void doCalcPar(DDS dds, DdTableDeal ddTableDeal, int vulnability)
    {
        Console.WriteLine($"CalcPar");
        tricks = new int[5, 4];

        var rc = dds.CalcPar( in ddTableDeal
                            , vulnability
                            , out DdTableResults tResults
                            , out ParResults results                        );

        Console.WriteLine(results.ParContractStrings);
        Console.WriteLine(results.ParScores);

        for (var trump = 0; trump <  5; trump++)
            for (var first = 0; first <  4; first++)
            {
                var decl =(first + 3) & 3;

                // record the number of tricks for declarer
                tricks[trump, decl] = tResults.ResultsTable[trump, decl];
            }

        dds.FreeMemory();

        DisplayTricks();
        //Console.WriteLine();
    }


    private static void doCalcPar(DDS dds, DdTableDealPBN ddTableDeal, int vulnability)
    {
        Console.WriteLine($"CalcPar PBN");
        tricks = new int[5, 4];

        var rc = dds.CalcPar( in ddTableDeal
                            , vulnability
                            , out DdTableResults tResults
                            , out ParResults results                        );

        Console.WriteLine(results.ParContractStrings);
        Console.WriteLine(results.ParScores);

        for (var trump = 0; trump <  5; trump++)
            for (var first = 0; first <  4; first++)
            {
                var decl =(first + 3) & 3;

                // record the number of tricks for declarer
                tricks[trump, decl] = tResults.ResultsTable[trump, decl];
            }

        dds.FreeMemory();

        DisplayTricks();
        //Console.WriteLine();
    }

    private static void doParSide(DDS dds, DdTableResults tResults, int vulnability)
    {
        Console.WriteLine($"ParSide -> ParResultsDealers");
        tricks = new int[5, 4];

        var rc = dds.ParSide( in tResults
                            , out ParResultsDealers results
                            , vulnability);

        Console.WriteLine(results[0].NumberOfContracts);
        Console.WriteLine(results[0].Score);
        Console.WriteLine(results[0].Contracts);
        Console.WriteLine();

        Console.WriteLine(results[1].NumberOfContracts);
        Console.WriteLine(results[1].Score);
        Console.WriteLine(results[1].Contracts);
        Console.WriteLine();

        dds.FreeMemory();
    }

    private static void doParAll(DDS dds, DdTableResults tResults, int vulnability)
    {
        Console.WriteLine($"ParAll -> ParResultsMasters ");
        tricks = new int[5, 4];

        var rc = dds.ParAll( in tResults
                           , out ParResultsMasters results
                           , vulnability);

        for (int s = 0; s <  2; s++)
        {
            Console.WriteLine($"Side {(s == 0 ? "N/S" : "E/W")}");
            Console.WriteLine(results[s].Number);
            Console.WriteLine(results[s].Score);

            for (int i = 0; i <  results[s].Number; i++)
            {
                var contract =results[s].Contracts[i];
                var d        = c[contract.Denomination];

                if (contract.UnderTricks >  0)
                    Console.WriteLine($"{contract.Level}{d}(-{contract.UnderTricks})");
                else
                    if (contract.OverTricks >  0)
                        Console.WriteLine($"{contract.Level}{d}(+{contract.OverTricks})");
                    else
                        Console.WriteLine($"{contract.Level}{d}(=)");
            }

            Console.WriteLine("");
        }

        TestData.parResultsMasters = results;
        dds.FreeMemory();
    }

    private static void doParDealer(DDS dds, DdTableResults tResults, int dealer, int vulnability)
    {
        Console.WriteLine($"ParDealer");
        tricks = new int[5, 4];

        var rc = dds.ParDealer( in tResults
                              , out ParResultsDealer results
                              , dealer
                              , vulnability
                              );

        Console.WriteLine(results.NumberOfContracts);
        Console.WriteLine(results.Score);
        Console.WriteLine(results.Contracts);
        Console.WriteLine("");

        dds.FreeMemory();
    }

    private static void doParDealerBothSides( DDS dds, DdTableResults tResults, int dealer
                                            , int vulnability)
    {
        Console.WriteLine($"DealerParBothSides");
        tricks = new int[5, 4];

        var rc = dds.DealerParBothSides( in tResults
                                       , out ParResultsMaster results
                                       , dealer
                                       , vulnability);

        Console.WriteLine(results.Number);
        Console.WriteLine(results.Score);

        for (int i = 0; i <  results.Number; i++)
            Console.WriteLine(results.Contracts[i]);

        Console.WriteLine("");
        TestData.parResultsMaster = results;
        dds.FreeMemory();
    }

    private static void doConvertToTextFormat(DDS dds, ParResultsMaster tResults)
    {
        Console.WriteLine($"ConvertToTextFormat ParResultsMaster");

        var rc = dds.ConvertToTextFormat( in tResults
                                        , out string str                                );

        Console.WriteLine(str);
        Console.WriteLine("");
        dds.FreeMemory();
    }

    private static void doConvertToTextFormat(DDS dds, ParResultsMasters tResults)
    {
        Console.WriteLine($"ConvertToTextFormat ParResultsMasters");

        var rc = dds.ConvertToTextFormat( in tResults
                                        , out ParTextResults str                                );

        Console.WriteLine(str.ParTextStrings);
        Console.WriteLine("");
        dds.FreeMemory();
    }

    private static void doAnalysePlay(DDS dds, Deal deal, ParResultsMasters tResults, PlayTraceBin ptrace)
    {
        Console.WriteLine($"AnalysePlay PlayTracesBin");

        var rc = dds.AnalysePlay( in deal
                                , in ptrace
                                , out SolvedPlay solved
                                , 0);

        for (int i = 0; i <= ptrace.NumberOfCards; i++)
            Console.WriteLine($"{i,2}: {solved.Tricks[i]}");

        Console.WriteLine("");
        dds.FreeMemory();
    }

    private static void doAnalysePlay(DDS dds, DealPBN deal, ParResultsMasters tResults, PlayTracePBN ptrace)
    {
        Console.WriteLine($"AnalysePlay PlayTracePBN");

        var rc = dds.AnalysePlay( in deal
                                , in ptrace
                                , out SolvedPlay solved
                                , 0);

        for (int i = 0; i <= ptrace.NumberOfPlayedCards; i++)
            Console.WriteLine($"{i,2}: {solved.Tricks[i]}");

        Console.WriteLine("");
        dds.FreeMemory();
    }

    private static void doAnalyseAllPlays(DDS dds, Boards boards, PlayTracesBin ptrace)
    {
        if (!isPerformanceTest)
            Console.WriteLine($"AnalysePlay PlayTracesBin");

        var rc = dds.AnalyseAllPlays( in boards
                                    , in ptrace
                                    , out SolvedPlays solved
                                    , 0);

        if (isPerformanceTest)
        {
            for (int i = 0; i <= ptrace.Plays[0].NumberOfCards; i++)
                Console.WriteLine($"{i,2}: {solved.Solved[0].Tricks[i]}");

            Console.WriteLine("");
        }

        dds.FreeMemory();
    }

    private static void doAnalyseAllPlays(DDS dds, BoardsPBN boards, PlayTracesPBN ptrace)
    {
        Console.WriteLine($"AnalyseAllPlay PlayTracesPBN");

        var rc = dds.AnalyseAllPlays( in boards
                                    , in ptrace
                                    , out SolvedPlays solved
                                    , 0);

        for (int i = 0; i <= ptrace.Plays[0].NumberOfPlayedCards; i++)
            Console.WriteLine($"{i,2}: {solved.Solved[0].Tricks[i]}");

        Console.WriteLine("");
        dds.FreeMemory();
    }

    private static void doGetDDSInfo(DDS dds)
    {
        Console.WriteLine($"GetDDSInfo");

        dds.GetDDSInfo(out DdsInfo info);

        Console.WriteLine($"DDS Info:");
        Console.WriteLine($"  Version:   {info.VersionString}");
        Console.WriteLine($"  System:    {info.System}");
        Console.WriteLine($"  Bits:      {info.NumberOfBits}");
        Console.WriteLine($"  Threading: {info.Threading}");
        Console.WriteLine($"  Threads:   {info.NumberOfThreads}");
        Console.WriteLine("");
    }

    private static void doErrorMessage(DDS dds, int rc)
    {
        Console.WriteLine($"Error Message");

        dds.ErrorMessage(rc, out string error);

        Console.WriteLine($"Error: {error}");
        Console.WriteLine("");
    }

    private static void DisplayTricks()
    {
        Console.WriteLine($"                       N  E  S  W");
        Console.WriteLine($"                      __ __ __ __");

        for (var denonination = 0; denonination <  5; denonination++)
        {
            Console.Write($"Tricks in {contracts[denonination],-10}: ");

            for (var declarer = 0; declarer <  4; declarer++)

                Console.Write($"{tricks[denonination, declarer],2} ");

            Console.WriteLine("");
        }

        Console.WriteLine();
    }

    private static void BenchmarkAnalyseAllPlaysBin(DDS dds)
    {
        Console.WriteLine("=== AnalyseAllPlaysBin P/Invoke Performance Benchmark ===");
        Console.WriteLine("(Run this in Release configuration for accurate results)\n");

        var boards        = TestData.boards;
        var playTracesBin = TestData.playTracesBin;
        try
        {
            var time1 = 0d;

            // Variant 1: Baseline - Cdecl with 'in' parameters (read-only reference)
            time1+= BenchmarkVariant1(dds, boards, playTracesBin);

            Console.WriteLine("\n=== Benchmark Complete ===");
            Console.Out.Flush();
        }

        catch (Exception ex)
        {
            Console.WriteLine($"\nERROR: {ex.GetType().Name}: {ex.Message}");
            Console.WriteLine($"Stack trace: {ex.StackTrace}");
            Console.Out.Flush();
        }
    }

    private static double BenchmarkVariant1(DDS dds, Boards boards, PlayTracesBin playTracesBin)
    {
        try
        {
            // Warmup iterations
            Console.WriteLine("\n--- Variant 1: in parameters all the way ---");
            Console.WriteLine("Warming up...");
            Console.Out.Flush();
            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();

            for (int i = 0; i <  5; i++)
                dds.AnalyseAllPlays(in boards, in playTracesBin, out SolvedPlays solved, 0);

            dds.FreeMemory();

            // Measure iterations
            Console.WriteLine($"Running {iterations} iterations...");

            var sw = Stopwatch.StartNew();

            for (int i = 0; i <  iterations; i++)
            {
                dds.AnalyseAllPlays(in boards, in playTracesBin, out SolvedPlays solved, 0);
                dds.FreeMemory();
            }

            sw.Stop();

            double avgMs        = sw.Elapsed.TotalMilliseconds / iterations;
            double opsPerSecond = 1000.0 / avgMs;

            Console.WriteLine($"Total time:     {sw.Elapsed.TotalMilliseconds:F2} ms");
            Console.WriteLine($"Average time:   {avgMs:F4} ms per call");
            Console.WriteLine($"Throughput:     {opsPerSecond:F2} calls/second");

            return opsPerSecond;
        }
        catch (Exception ex)
        {
            Console.WriteLine($"ERROR in Variant1: {ex.Message}");

            if (ex.InnerException != null)
                Console.WriteLine($"Inner: {ex.InnerException.Message}");
        }

        return 0d;
    }
}

