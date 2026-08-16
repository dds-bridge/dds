using System.Runtime.InteropServices;
using System.Text;

namespace DDS_Core.Native;

internal static class DdsNative
{
    // One native library for every platform. .NET's probing supplies the "lib"
    // prefix and the per-OS extension, so this single name resolves
    // libdds.dylib, libdds.so, and dds.dll.
    private const string DllName = "dds";

    // Runs before this type's first P/Invoke, so the DDS_LIBRARY_PATH override
    // is always in place without consumers having to call anything.
    static DdsNative() => DdsNativeResolver.Register();

    // The modern context entry points below bind the pure-C shim (dds_c_*)
    // rather than the reference-taking dds_* functions in dds_api.hpp. Only the
    // shim is exported by the shared library on Linux and macOS; the managed
    // method names are kept as-is so call sites are unaffected.

    #region ====== Version 3 specific methods ======
        #region ===== Solver Context Management ======
            [DllImport(DllName, EntryPoint = "dds_c_create_solvercontext_default", CallingConvention = CallingConvention.Cdecl)]

            internal static extern SolverContextHandle dds_create_solvercontext_default();

            // SolverConfig is passed as scalars: the shim is pointer-only and
            // POD-only by design, so no struct crosses the ABI boundary.
            [DllImport(DllName, EntryPoint = "dds_c_create_solvercontext", CallingConvention = CallingConvention.Cdecl)]
            internal static extern SolverContextHandle dds_create_solvercontext( int ttKind
                                                                              , int defMB
                                                                              , int maxMB);

            [DllImport(DllName, EntryPoint = "dds_c_destroy_solvercontext", CallingConvention = CallingConvention.Cdecl)]
            internal static extern void dds_destroy_solvercontext(IntPtr ctx);

            // TTKind is `enum TTKind : int`, which marshals as a plain int and
            // matches the shim's int tt_kind parameter, so the managed
            // signature is unchanged.
            [DllImport(DllName, EntryPoint = "dds_c_configure_tt", CallingConvention = CallingConvention.Cdecl)]
            internal static extern void dds_configure_tt( SolverContextHandle ctx
                                                        , TTKind kind
                                                        , int defMB
                                                        , int maxMB);

            [DllImport(DllName, EntryPoint = "dds_c_resize_tt", CallingConvention = CallingConvention.Cdecl)]
            internal static extern void dds_resize_tt( SolverContextHandle ctx
                                                     , int defMB
                                                     , int maxMB);

            [DllImport(DllName, EntryPoint = "dds_c_clear_tt", CallingConvention = CallingConvention.Cdecl)]
            internal static extern void dds_clear_tt(SolverContextHandle ctx);

            [DllImport(DllName, EntryPoint = "dds_c_reset_for_solve", CallingConvention = CallingConvention.Cdecl)]
            internal static extern void dds_reset_for_solve(SolverContextHandle ctx);

            [DllImport(DllName, EntryPoint = "dds_c_reset_best_moves_lite", CallingConvention = CallingConvention.Cdecl)]
            internal static extern void dds_reset_best_moves_lite(SolverContextHandle ctx);

            // The shim documents msg as NUL-terminated UTF-8; be explicit rather
            // than relying on the platform-dependent CharSet.Ansi default.
            [DllImport(DllName, EntryPoint = "dds_c_log_append", CallingConvention = CallingConvention.Cdecl)]
            internal static extern void dds_log_append( SolverContextHandle ctx
                                                      , [MarshalAs(UnmanagedType.LPUTF8Str)] string msg);

            [DllImport(DllName, EntryPoint = "dds_c_log_clear", CallingConvention = CallingConvention.Cdecl)]
            internal static extern void dds_log_clear( SolverContextHandle ctx);
        #endregion

        #region ====== SolverContext methods ======
            [DllImport(DllName, EntryPoint = "dds_c_solve_board", CallingConvention = CallingConvention.Cdecl)]
            public static extern int dds_solve_board( SolverContextHandle ctx
                                                    , in Deal dl
                                                    , int target
                                                    , int solutions
                                                    , int mode
                                                    , out FutureTricks fut);

            #region Call_dd
                [DllImport(DllName, EntryPoint = "dds_c_calc_dd_table", CallingConvention = CallingConvention.Cdecl)]
                public static extern int dds_calc_dd_table( SolverContextHandle ctx
                                                      , in DdTableDeal table_deal
                                                      , out DdTableResults table_results);

                [DllImport(DllName, EntryPoint = "dds_c_calc_dd_table_pbn", CallingConvention = CallingConvention.Cdecl)]
                public static extern int dds_calc_dd_table_pbn( SolverContextHandle ctx
                                                          , in DdTableDealPBN table_deal_pbn
                                                          , out DdTableResults table_results);
            #endregion

            #region Call_par
                [DllImport(DllName, EntryPoint = "dds_c_calc_par", CallingConvention = CallingConvention.Cdecl)]
                public static extern int dds_calc_par( SolverContextHandle ctx
                                                 , in DdTableDeal table_deal
                                                 , int vulnerable
                                                 , out DdTableResults table_results
                                                 , out ParResults par_results);
            #endregion
        #endregion
    #endregion

    #region ====== Configuration and Resource Management ======
        // The C symbol SetMaxThreads is deprecated and now a thin alias of
        // InitializeStaticMemory; the userThreads argument is ignored.
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void SetMaxThreads( int userThreads);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int SetThreading(int code);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void SetResources( int maxMemoryMB
                                              , int maxThreads);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void FreeMemory();
    #endregion

    #region ====== Single Board Solving ======
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int SolveBoard( in Deal dl
                                           , int target
                                           , int solutions
                                           , int mode
                                           , out FutureTricks fut
                                           , int threadIndex);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int SolveBoardPBN( in DealPBN dlpbn
                                              , int target
                                              , int solutions
                                              , int mode
                                              , out FutureTricks fut
                                              , int thrId);
    #endregion

    #region ====== Multiple Board Solving ======
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int SolveAllBoards( in BoardsPBN bop
                                               , out SolvedBoards solved);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int SolveAllBoardsBin( in Boards bop
                                                  , out SolvedBoards solved);

        //[DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        //public static extern int SolveAllChunks( in BoardsPBN bop
        //                                       , out SolvedBoards solved
        //                                       , int chunkSize);

        //[DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        //public static extern int SolveAllChunksBin( in Boards bop
        //                                          , out SolvedBoards solved
        //                                          , int chunkSize);

        //[DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        //public static extern int SolveAllChunksPBN( in BoardsPBN bop
        //                                          , out SolvedBoards solved
        //                                          , int chunkSize);
    #endregion

    #region ====== Double Dummy Table Calculation ======
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int CalcDDtable( in DdTableDeal tableDeal
                                            , out DdTableResults table);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int CalcDDtablePBN( in DdTableDealPBN tableDealPBN
                                               , out DdTableResults table);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int CalcAllTables( in DdTableDeals dealsp
                                              , int mode
                                              , intArray5 trumpFilter
                                              , out DdTablesResult resp
                                              , out AllParResults presp);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int CalcAllTablesPBN( in DdTableDealsPBN dealsp
                                                 , int mode
                                                 , in intArray5 trumpFilter
                                                 , out DdTablesResult resp
                                                 , out AllParResults presp);
    #endregion

    #region ====== Par Score Calculation ======
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int Par( in DdTableResults table
                                    , out ParResults pres
                                    , int vulnerable);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int CalcPar( in DdTableDeal tableDeal
                                        , int vulnerable
                                        , out DdTableResults table
                                        , out ParResults pres);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int CalcParPBN( in DdTableDealPBN tableDealPBN
                                           , out DdTableResults table
                                           , int vulnerable
                                           , out ParResults pres);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int SidesPar( in DdTableResults table
                                         , out ParResultsDealers sidesRes
                                         , int vulnerable);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int DealerPar( in DdTableResults table
                                          , out ParResultsDealer pres
                                          , int dealer
                                          , int vulnerable);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int DealerParBin( in DdTableResults table
                                             , out ParResultsMaster pres
                                             , int dealer
                                             , int vulnerable);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int SidesParBin( in DdTableResults table
                                            , out ParResultsMasters sidesRes
                                            , int vulnerable);
    #endregion

    #region ====== Par Text Conversion ======
      [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern int ConvertToDealerTextFormat( in ParResultsMaster pres
                                                          , StringBuilder resp);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ConvertToSidesTextFormat( in ParResultsMasters pres
                                                         , out ParTextResults resp);
    #endregion

    #region ====== Play Analysis ======
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int AnalysePlayBin( in Deal dl
                                               , in PlayTraceBin play
                                               , out SolvedPlay solved
                                               , int thrId);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern int AnalysePlayPBN( in DealPBN dlPBN
                                               , in PlayTracePBN playPBN
                                               , out SolvedPlay solved
                                               , int thrId);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int AnalyseAllPlaysBin( in Boards bop
                                                   , in PlayTracesBin plp
                                                   , out SolvedPlays solved
                                                   , int chunkSize);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int AnalyseAllPlaysPBN( in BoardsPBN bopPBN
                                                   , in PlayTracesPBN plpPBN
                                                   , out SolvedPlays solved
                                                   , int chunkSize);
    #endregion

    #region ====== Utility Functions ======
        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void GetDDSInfo(out DdsInfo info);

        [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern void ErrorMessage( int code
                                              , StringBuilder line);
    #endregion
}

