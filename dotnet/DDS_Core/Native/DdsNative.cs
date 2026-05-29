using System;
using System.Runtime.InteropServices;
using System.Text;
using DDS_Core;

namespace DDS_Core.Native;

internal static class DdsNative
{
    private const string DllName = "dds_native";

    #region ====== Configuration and Resource Management ======
        [DllImport(DllName)]
        public static extern void SetMaxThreads(int userThreads);

        [DllImport(DllName)]
        public static extern int SetThreading(int code);

        [DllImport(DllName)]
        public static extern void SetResources( int maxMemoryMB
                                              , int maxThreads);

        [DllImport(DllName)]
        public static extern void FreeMemory();
    #endregion

    #region ====== Single Board Solving ======
        [DllImport(DllName)]
        public static extern int SolveBoard( in Deal dl
                                           , int target
                                           , int solutions
                                           , int mode
                                           , out FutureTricks fut
                                           , int threadIndex);

        [DllImport(DllName)]
        public static extern int SolveBoardPBN( in DealPBN dlpbn
                                              , int target
                                              , int solutions
                                              , int mode
                                              , out FutureTricks fut
                                              , int thrId);
    #endregion

    #region ====== Multiple Board Solving ======
        [DllImport(DllName)]
        public static extern int SolveAllBoards( in Boards bop
                                               , out SolvedBoards solved);

        [DllImport(DllName)]
        public static extern int SolveAllBoardsBin( in Boards bop
                                                  , out SolvedBoards solved);

        [DllImport(DllName)]
        public static extern int SolveAllChunks( in BoardsPBN bop
                                               , out SolvedBoards solved
                                               , int chunkSize);

        [DllImport(DllName)]
        public static extern int SolveAllChunksBin( in Boards bop
                                                  , out SolvedBoards solved
                                                  , int chunkSize);

        [DllImport(DllName)]
        public static extern int SolveAllChunksPBN( in BoardsPBN bop
                                                  , out SolvedBoards solved
                                                  , int chunkSize);
    #endregion

    #region ====== Double Dummy Table Calculation ======
        [DllImport(DllName)]
        public static extern int CalcDDtable( in DdTableDeal tableDeal
                                            , out DdTableResults table);

        [DllImport(DllName)]
        public static extern int CalcDDtablePBN( in DdTableDealPBN tableDealPBN
                                               , out DdTableResults table);

        [DllImport(DllName)]
        public static extern int CalcAllTables( in DdTableDeals dealsp
                                              , int mode
                                              , int[] trumpFilter //TODO: Tjekke denne    
                                              , out DdTablesRes resp
                                              , out AllParResults presp);

        [DllImport(DllName)]
        public static extern int CalcAllTablesPBN( in DdTableDealsPBN dealsp
                                                 , int mode
                                                 , int[] trumpFilter
                                                 , out DdTablesRes resp
                                                 , out AllParResults presp);
    #endregion

    #region ====== Par Score Calculation ======
        [DllImport(DllName)]
        public static extern int Par( in DdTableResults table
                                    , out ParResults pres
                                    , int vulnerable);

        [DllImport(DllName)]
        public static extern int CalcPar( in DdTableDeal tableDeal
                                        , int vulnerable
                                        , out DdTableResults table
                                        , out ParResults pres);

        [DllImport(DllName)]
        public static extern int CalcParPBN( in DdTableDealPBN tableDealPBN
                                           , out DdTableResults table
                                           , int vulnerable
                                           , out ParResults pres);

        [DllImport(DllName)]
        public static extern int SidesPar( in DdTableResults table
                                         , [Out] ParResultsDealer[] sidesRes
                                         , int vulnerable);

        [DllImport(DllName)]
        public static extern int DealerPar( in DdTableResults table
                                          , out ParResultsDealer pres
                                          , int dealer
                                          , int vulnerable);

        [DllImport(DllName)]
        public static extern int DealerParBin( in DdTableResults table
                                             , out ParResultsMaster pres
                                             , int dealer
                                             , int vulnerable);

        [DllImport(DllName)]
        public static extern int SidesParBin( in DdTableResults table
                                            , [Out] ParResultsMaster[] sidesRes
                                            , int vulnerable);
    #endregion

    #region ====== Par Text Conversion ======
        [DllImport(DllName, CharSet = CharSet.Ansi)]
        public static extern int ConvertToDealerTextFormat( in ParResultsMaster pres
                                                          , StringBuilder resp); //TODO: Tjek alle StringBuilder

        [DllImport(DllName)]
        public static extern int ConvertToSidesTextFormat( in ParResultsMaster pres
                                                         , out ParTextResults resp);
    #endregion

    #region ====== Play Analysis ======
        [DllImport(DllName)]
        public static extern int AnalysePlayBin( in Deal dl
                                               , in PlayTraceBin play
                                               , out SolvedPlay solved
                                               , int thrId);

        [DllImport(DllName, CharSet = CharSet.Ansi)]
        public static extern int AnalysePlayPBN( in DealPBN dlPBN
                                               , in PlayTracePBN playPBN
                                               , out SolvedPlay solved
                                               , int thrId);

        [DllImport(DllName)]
        public static extern int AnalyseAllPlaysBin( in Boards bop
                                                   , in PlayTracesBin plp
                                                   , out SolvedPlays solved
                                                   , int chunkSize);

        [DllImport(DllName, CharSet = CharSet.Ansi)]
        public static extern int AnalyseAllPlaysPBN( in BoardsPBN bopPBN
                                                   , in PlayTracesPBN plpPBN
                                                   , out SolvedPlays solved
                                                   , int chunkSize);
    #endregion

    #region ====== Utility Functions ======
        [DllImport(DllName)]
        public static extern void GetDDSInfo(out DDSInfo info);

        [DllImport(DllName, CharSet = CharSet.Ansi)]
        public static extern void ErrorMessage( int code
                                              , StringBuilder line);
    #endregion
}

