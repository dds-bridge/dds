using System.Diagnostics;
using System.Text;
using DDS_Core.Helpers;
using DDS_Core.Native;

namespace DDS_Core;

public class DDS
{
    #region ====== Configuration and Resource Management ======
        /// <summary>
        /// Sets the maximum number of threads used by the solver.
        /// </summary>
        /// <remarks>
        /// <para>
        /// <b>Deprecated.</b> In the modern C++ API, thread count is controlled by the
        /// embedding application — typically by creating one <c>SolverContext</c>
        /// instance per worker thread.
        /// </para>
        /// <para>
        /// New code should create and destroy <c>SolverContext</c> instances in the
        /// application rather than calling this function. See <c>docs/api_migration.md</c>
        /// for examples of the modern API.
        /// </para>
        /// <para>
        /// This function is part of the legacy C API and is maintained for backward
        /// compatibility. It has no direct equivalent in the modern API, where both
        /// threading and transposition‑table memory limits are configured per instance
        /// via <c>SolverContext</c> and <c>SolverConfig</c>.
        /// </para>
        /// </remarks>
        /// <param name="userThreads">Maximum number of threads to use.</param>
        [Obsolete("Use SolverContext instead.")]
        public void SetMaxThreads(int userThreads) => DdsNative.SetMaxThreads(userThreads);

        /// <summary>
        /// Sets the threading backend used by the solver.
        /// </summary>
        /// <remarks>
        /// <para>
        /// <b>Deprecated.</b> Use <c>SolverContext</c> instead — threading is implicit
        /// (one context per thread). See <c>docs/api_migration.md</c> for modern C++ API examples.
        /// </para>
        /// <para>
        /// This function is part of the legacy C API and is maintained for backward
        /// compatibility. The modern C++ API does not require explicit threading
        /// configuration; instead, create one <c>SolverContext</c> instance per thread.
        /// </para>
        /// </remarks>
        /// <param name="code">Threading backend code (see documentation).</param>
        /// <returns>Error code (<c>RETURN_NO_FAULT</c> on success).</returns>
        [Obsolete("Use SolverContext instead.")]
        public int SetThreading(in int code)
        {
            var rc = DdsNative.SetThreading(code);

            ThrowIfError(rc, nameof(SetThreading));
            return rc;
        }

        /// <summary>
        /// Sets memory and thread resources for the solver.
        /// </summary>
        /// <remarks>
        /// <para>
        /// <b>Deprecated.</b> Use <c>SolverContext</c> with <c>SolverConfig</c> instead.
        /// See <c>docs/api_migration.md</c> for modern C++ API examples.
        /// </para>
        /// <para>
        /// This function is part of the legacy C API and is maintained for backward
        /// compatibility. New code should use the modern C++ API with <c>SolverContext</c>,
        /// which provides per‑instance configuration through <c>SolverConfig</c>.
        /// </para>
        /// </remarks>
        /// <param name="maxMemoryMB">Maximum memory in megabytes.</param>
        /// <param name="maxThreads">Maximum number of threads.</param>
        [Obsolete("Use SolverContext instead.")]
        public void SetResources(in int maxMemoryMB, in int maxThreads) => DdsNative.SetResources(maxMemoryMB, maxThreads);

        /// <summary>
        /// Frees memory used by the solver.
        /// </summary>
        /// <remarks>
        /// <para>
        /// <b>Deprecated.</b> Use <c>SolverContext</c> RAII instead — cleanup is automatic.
        /// See <c>docs/api_migration.md</c> for modern C++ API examples.
        /// </para>
        /// <para>
        /// This function is part of the legacy C API and is maintained for backward
        /// compatibility. The modern C++ API uses RAII (<i>Resource Acquisition Is
        /// Initialization</i>) through <c>SolverContext</c>, which automatically cleans up
        /// resources when the context goes out of scope.
        /// </para>
        /// </remarks>
        [Obsolete("Use SolverContext instead.")]
        public void FreeMemory() => DdsNative.FreeMemory();
    #endregion

    #region ====== Single Board Solving ======
        /// <summary>
        /// Solves a single bridge <c>Deal</c> using double dummy analysis.
        /// </summary>
        /// <param name="dl">The deal to analyze.</param>
        /// <param name="target">Target number of tricks.</param>
        /// <param name="solutions">Solution mode (1 = best, 2 = all, etc.).</param>
        /// <param name="mode">Analysis mode.</param>
        /// <param name="fut">The result.</param>
        /// <param name="threadIndex">Index of the thread to use.</param>
        /// <returns>Error code (<c>RETURN_NO_FAULT</c> on success).</returns>
        [Obsolete("Use SolverContext instead.")]
        public int SolveBoard( in Deal dl
                             , in int target
                             , in int solutions
                             , in int mode
                             , out FutureTricks fut
                             , int threadIndex = 0)
        {
            var rc = DdsNative.SolveBoard( dl
                                         , target
                                         , solutions
                                         , mode
                                         , out fut
                                         , threadIndex);

            ThrowIfError(rc, nameof(SolveBoard));
            return rc;
        }

        /// <summary>
        /// Solves a single bridge deal in PBN format using double dummy analysis.
        /// </summary>
        /// <param name="pbn">The PBN deal to analyze.</param>
        /// <param name="target">Target number of tricks.</param>
        /// <param name="solutions">Solution mode.</param>
        /// <param name="mode">Analysis mode.</param>
        /// <param name="fut">The result.</param>
        /// <param name="threadIndex">Index of the thread to use.</param>
        /// <returns>Error code (<c>RETURN_NO_FAULT</c> on success).</returns>
        [Obsolete("Use SolverContext instead.")]
        public int SolveBoard( in DealPBN pbn
                             , in int target
                             , in int solutions
                             , in int mode
                             , out FutureTricks fut
                             , int threadIndex = 0)
        {
            var rc = DdsNative.SolveBoardPBN( pbn
                                            , target
                                            , solutions
                                            , mode
                                            , out fut
                                            , threadIndex);

            ThrowIfError(rc, nameof(SolveBoard));
            return rc;
        }
    #endregion

    #region ====== Multiple Board Solving ======
        /// <summary>
        /// Solves multiple bridge deals in PBN format.
        /// </summary>
        /// <param name="boards">Multiple PBN deals.</param>
        /// <param name="solved">The results for solved boards.</param>
        /// <returns>Error code (<c>RETURN_NO_FAULT</c> on success).</returns>
        public int SolveAllBoards( in BoardsPBN boards
                                 , out SolvedBoards solved)
        {
            solved = default;
            //Note: To step into c++ code you must set a break in c++?
            var rc = DdsNative.SolveAllBoards( boards
                                             , out solved);

            ThrowIfError(rc, nameof(SolveAllBoards));
            return rc;
        }

        /// <summary>
        /// Solves multiple bridge deals in binary format.
        /// </summary>
        /// <param name="bop">Multiple deals.</param>
        /// <param name="solvedp">The results for solved boards.</param>
        /// <returns>Error code (<c>RETURN_NO_FAULT</c> on success).</returns>
        [Obsolete("Use SolverContext instead.")]
        public int SolveAllBoards( in Boards bop
                                 , out SolvedBoards solved)
        {
            solved = default;

            //Note: To step into c++ code you must set a break in c++?
            var rc = DdsNative.SolveAllBoardsBin( bop
                                                , out solved);

            ThrowIfError(rc, nameof(SolveAllBoards));
            return rc;
        }
    #endregion

    #region ====== Double Dummy Table Calculation ======
        /// <summary>
        /// Calculates the double dummy table for a given deal.
        /// </summary>
        /// <param name="tableDeal">Deal for which to calculate the table.</param>
        /// <param name="table">The result table.</param>
        /// <returns>Error code (<c>RETURN_NO_FAULT</c> on success).</returns>
        public int CalcDdTable( in DdTableDeal deal
                              , out DdTableResults table)
        {
            var rc = DdsNative.CalcDDtable( deal
                                          , out table);

            ThrowIfError(rc, nameof(CalcDdTable));
            return rc;
        }

        /// <summary>
        /// Calculates the double dummy table for a PBN deal.
        /// </summary>
        /// <param name="tableDealPBN">PBN deal for which to calculate the table.</param>
        /// <param name="table">The result table.</param>
        /// <returns>Error code (<c>RETURN_NO_FAULT</c> on success).</returns>
        public int CalcDdTable( in DdTableDealPBN tableDealPBN
                              , out DdTableResults table)
        {
            var rc = DdsNative.CalcDDtablePBN( tableDealPBN
                                             , out table);

            ThrowIfError(rc, nameof(CalcDdTable));
            return rc;
        }

        /// <summary>
        /// Calculates double dummy tables and par contracts for multiple deals.
        /// </summary>
        /// <param name="deals">Multiple deals.</param>
        /// <param name="mode">Analysis mode.</param>
        /// <param name="trumpFilter">Array of trump suit filters.</param>
        /// <param name="resTables">The result tables.</param>
        /// <param name="parResults">The par results.</param>
        /// <returns>Error code (<c>RETURN_NO_FAULT</c> on success).</returns>
        public int CalcAllTables( in DdTableDeals deals
                                , in int mode
                                , in intArray5 trumpFilter
                                , out DdTablesResult resTables
                                , out AllParResults parResults)
        {
            var rc = DdsNative.CalcAllTables( deals
                                            , mode
                                            , trumpFilter
                                            , out resTables
                                            , out parResults);

            ThrowIfError(rc, nameof(CalcAllTables));
            return rc;
        }

        /// <summary>
        /// Calculates double dummy tables and par contracts for multiple PBN deals.
        /// </summary>
        /// <param name="deals">Multiple PBN deals.</param>
        /// <param name="mode">Analysis mode.</param>
        /// <param name="trumpFilter">Array of trump suit filters.</param>
        /// <param name="ResTables">The result tables.</param>
        /// <param name="parResults">The par results.</param>
        /// <returns>Error code (<c>RETURN_NO_FAULT</c> on success).</returns>
        public int CalcAllTables( in DdTableDealsPBN dealsp
                                , in int mode
                                , in intArray5 trumpFilter
                                , out DdTablesResult ResTables
                                , out AllParResults parResults)
        {
            var rc = DdsNative.CalcAllTablesPBN( dealsp
                                               , mode
                                               , trumpFilter
                                               , out ResTables
                                               , out parResults);

            ThrowIfError(rc, nameof(CalcAllTables));
            return rc;
        }
    #endregion

    #region ====== Par Score Calculation ======
        /// <summary>
        /// Computes the par score and contracts for both sides for a given double dummy results.
        ///
        /// This function analyzes the double dummy table results and determines the par score
        /// and contracts for both North-South and East-West, based on vulnerability.
        /// </summary>
        /// <param name="table"></param>
        /// <param name="vulnerable"></param>
        /// <param name="pres"></param>
        /// <returns></returns>
        public int Par( in DdTableResults table
                      , in int vulnerable
                      , out ParResults pres
                      )
        {
            var rc = DdsNative.Par( table
                                  , out pres
                                  , vulnerable);

            ThrowIfError(rc, nameof(Par));
            return rc;
        }

        /// <summary>
        /// Calculates par score and contracts for a deal table.
        /// </summary>
        /// <remarks>
        /// <para>
        /// Computes the double dummy table for the given deal, then calculates par score
        /// and contracts based on vulnerability. This overload creates a temporary
        /// <c>SolverContext</c> internally.
        /// </para>
        /// <para>
        /// This function is equivalent to calling <c>CalcDDtable</c> followed by
        /// <c>Par</c> in the legacy C API.
        /// </para>
        /// </remarks>
        /// <param name="tableDeal">
        /// Deal represented as card holdings for each hand.
        /// </param>
        /// <param name="vulnerable">
        /// Vulnerability (0=None, 1=Both, 2=NS, 3=EW).
        /// </param>
        /// <param name="tableResults">
        /// Output: double dummy table results.
        /// </param>
        /// <param name="parResults">
        /// Output: par score and contract strings.
        /// </param>
        /// <returns>
        /// Error code (<c>RETURN_NO_FAULT</c> on success).
        /// </returns>
        public int CalcPar( in DdTableDeal tableDeal
                          , in int vulnerable
                          , out DdTableResults tableResults
                          , out ParResults parResults)
        {
            var rc = DdsNative.CalcPar( tableDeal
                                      , vulnerable
                                      , out tableResults
                                      , out parResults);
            ThrowIfError(rc, nameof(CalcPar));
            return rc;
        }

        /// <summary>
        /// Calculates par score and contracts for a deal table in PBN format.
        /// </summary>
        /// <remarks>
        /// <para>
        /// Computes the double dummy table for the given deal, then calculates par score
        /// and contracts based on vulnerability. This overload creates a temporary
        /// <c>SolverContext</c> internally.
        /// </para>
        /// <para>
        /// This function is equivalent to calling <c>CalcDDtable</c> followed by
        /// <c>Par</c> in the legacy C API.
        /// </para>
        /// </remarks>
        /// <param name="tableDealPBN">
        /// Deal represented as a PBN string for each hand.
        /// </param>     
        /// <param name="vulnerable">
        /// Vulnerability (0=None, 1=Both, 2=NS, 3=EW).
        /// </param>
        /// <param name="tableResults">
        /// Output: double dummy table results.
        /// </param>
        /// <param name="parResults">
        /// Output: par score and contract strings.
        /// </param>
        /// <returns>
        /// Error code (<c>RETURN_NO_FAULT</c> on success).
        /// </returns>
        public int CalcPar( in DdTableDealPBN tableDealPBN
                          , in int vulnerable
                          , out DdTableResults tableResults
                          , out ParResults parResults)
        {
            var rc = DdsNative.CalcParPBN( tableDealPBN
                                         , out tableResults
                                         , vulnerable
                                         , out parResults);

            ThrowIfError(rc, nameof(CalcPar));
            return rc;
        }

        /// <summary>
        /// Calculates par score and contracts for both sides based on the double dummy table results.
        /// </summary>
        /// <param name="table">Double dummy table results.</param>
        /// <param name="vulnerable">Vulnerability (0=None, 1=Both, 2=NS, 3=EW).</param>
        /// <param name="sidesRes">Output: par results for both sides.</param>
        /// <returns>Error code (<c>RETURN_NO_FAULT</c> on success).</returns>
        public int ParSide( in DdTableResults table
                          , in int vulnerable
                          , out ParResultsDealers sidesRes
                          )
        {
            var rc = DdsNative.SidesPar( table
                                       , out sidesRes
                                       , vulnerable);

            ThrowIfError(rc, nameof(ParSide));
            return rc;
        }

        /// <summary>
        /// Calculates par score and contracts for a specific dealer and vulnerability.
        /// </summary>
        /// <param name="table"></param>
        /// <param name="dealer"></param>
        /// <param name="vulnerable"></param>
        /// <param name="pres"></param>
        /// <returns></returns>
        public int ParDealer( in DdTableResults table
                            , in int dealer
                            , in int vulnerable
                            , out ParResultsDealer pres
                            )
        {
            var rc = DdsNative.DealerPar( table
                                        , out pres
                                        , dealer
                                        , vulnerable);

            ThrowIfError(rc, nameof(ParDealer));
            return rc;
        }

        /// <summary>
        /// Calculates par score and contract types for both sides for a specific dealer and vulnerability.
        /// </summary>
        /// <param name="table"></param>
        /// <param name="dealer"></param>
        /// <param name="vulnerable"></param>
        /// <param name="pres"></param>
        /// <returns></returns>
        public int DealerParBothSides( in DdTableResults table
                                     , in int dealer
                                     , in int vulnerable
                                     , out ParResultsMaster pres
                                     )
        {
            var rc = DdsNative.DealerParBin( table
                                           , out pres
                                           , dealer
                                           , vulnerable);

            ThrowIfError(rc, nameof(DealerParBothSides));
            return rc;
        }

        /// <summary>
        /// calculates par score and contract types for both sides based on the double dummy table results.
        /// </summary>
        /// <param name="table">Double dummy table results.</param>
        /// <param name="vulnerable">Vulnerability (0=None, 1=Both, 2=NS, 3=EW).</param>
        /// <param name="sidesRes">Output: par results for both sides.</param>
        /// <returns>Error code (<c>RETURN_NO_FAULT</c> on success).</returns>
        public int ParAll( in DdTableResults table
                         , in int vulnerable
                         , out ParResultsMasters sidesRes
                         )
        {
            var rc = DdsNative.SidesParBin( table
                                          , out sidesRes
                                          , vulnerable);

            ThrowIfError(rc, nameof(ParAll));
            return rc;
        }
    #endregion

    #region ====== Play Analysis ======
        /// <summary>
        /// Analyzes a play trace for a given deal and determines the optimal line of play.
        /// </summary>
        /// <param name="dl">The deal to analyze.</param>
        /// <param name="play">The play trace to analyze.</param>
        /// <param name="thrId">The thread ID for parallel processing.</param>
        /// <param name="solved">The result of the analysis, including optimal line and tricks.</param>
        /// <returns>Error code (<c>RETURN_NO_FAULT</c> on success).</returns>
        public int AnalysePlay( in Deal dl
                              , in PlayTraceBin play
                              , in int thrId
                              , out SolvedPlay solved
                              )
        {
            var rc = DdsNative.AnalysePlayBin( dl
                                             , in play
                                             , out solved
                                             , thrId);
            ThrowIfError(rc, nameof(AnalysePlay));
            return rc;
        }

        /// <summary>
        /// analyzes a play trace for a given deal in PBN format and determines the optimal line of play.
        /// </summary>
        /// <param name="dlPBN">The PBN deal to analyze.</param>
        /// <param name="playPBN">The play trace in PBN format to analyze.</param>
        /// <param name="thrId">The thread ID for parallel processing.</param>
        /// <param name="solved">The result of the analysis, including optimal line and tricks.</param>
        /// <returns>Error code (<c>RETURN_NO_FAULT</c> on success).</returns>
        public int AnalysePlay( in DealPBN dlPBN
                              , in PlayTracePBN playPBN
                              , in int thrId
                              , out SolvedPlay solved
                              )
        {
            var rc = DdsNative.AnalysePlayPBN( dlPBN
                                             , in playPBN
                                             , out solved
                                             , thrId);
            ThrowIfError(rc, nameof(AnalysePlay));
            return rc;
        }

        /// <summary>
        /// Analyzes all play traces for a given set of boards and determines the optimal lines of play.
        /// </summary>
        /// <param name="bop">The boards to analyze.</param>
        /// <param name="plp">The play traces to analyze.</param>
        /// <param name="chunkSize">The chunk size for parallel processing.</param>
        /// <param name="solved">The result of the analysis, including optimal lines and tricks.</param>
        /// <returns>Error code (<c>RETURN_NO_FAULT</c> on success).</returns>
        public int AnalyseAllPlays( in Boards bop
                                  , in PlayTracesBin plp
                                  , in int chunkSize
                                  , out SolvedPlays solved
                                  )
        {
            solved = new();

            var rc = DdsNative.AnalyseAllPlaysBin( bop
                                                 , plp
                                                 , out solved
                                                 , chunkSize);

            ThrowIfError(rc, nameof(AnalyseAllPlays));
            return rc;
        }

        /// <summary>
        /// Analyzes all play traces for a given set of boards in PBN format and determines the optimal lines of play.
        /// </summary>
        /// <param name="bopPBN">The boards in PBN format to analyze.</param>
        /// <param name="plpPBN">The play traces in PBN format to analyze.</param>
        /// <param name="chunkSize">The chunk size for parallel processing.</param>
        /// <param name="solved">The result of the analysis, including optimal lines and tricks.</param>
        /// <returns>Error code (<c>RETURN_NO_FAULT</c> on success).</returns>
        public int AnalyseAllPlays( in BoardsPBN bopPBN
                                  , in PlayTracesPBN plpPBN
                                  , in int chunkSize
                                  , out SolvedPlays solved
                                  )
        {
            solved = new();
            var rc = DdsNative.AnalyseAllPlaysPBN( bopPBN
                                                 , plpPBN
                                                 , out solved
                                                 , chunkSize);
            ThrowIfError(rc, nameof(AnalyseAllPlays));
            return rc;
        }
    #endregion

    #region ====== Utility Functions ======
        #region ====== Par Text Conversion ======
            /// <summary>
            /// Converts par results to a human-readable text format.
            /// </summary>
            /// <param name="pres">Par results to convert.</param>
            /// <param name="resp">Output: human-readable par results.</param>
            /// <returns>Error code (<c>RETURN_NO_FAULT</c> on success).</returns>
            public int ConvertToTextFormat( in ParResultsMaster pres
                                          , out string resp)
            {
                var str = new StringBuilder(80);
                var rc  = DdsNative.ConvertToDealerTextFormat( pres
                                                             , str);
                ThrowIfError(rc, nameof(ConvertToTextFormat));
                resp = str.ToString();
                return rc;
            }

            /// <summary>
            /// converts par results for both sides to a human-readable text format.
            /// </summary>
            /// <param name="pres">Par results for both sides to convert.</param>
            /// <param name="resp">Output: human-readable par results for both sides.</param>
            /// <returns>Error code (<c>RETURN_NO_FAULT</c> on success).</returns>
            public int ConvertToTextFormat( in ParResultsMasters pres
                                          , out ParTextResults resp)
            {
                var rc = DdsNative.ConvertToSidesTextFormat( pres
                                                           , out resp);
                ThrowIfError(rc, nameof(ConvertToTextFormat));
                return rc;
            }
        #endregion

        /// <summary>
        /// Retrieves information about the DDS library.
        /// </summary>
        /// <param name="info">The DDS information.</param>
        public void GetDDSInfo(out DdsInfo info)
        {
            DdsNative.GetDDSInfo(out info);
        }

        /// <summary>
        /// Retrieves the error message corresponding to a given error code.
        /// </summary>
        /// <param name="code">The error code for which to retrieve the message.</param>
        /// <param name="line">Output: the error message corresponding to the provided code.</param>
        public void ErrorMessage( in int code
                                , out string line)
        {
            var str = new StringBuilder(80);
            DdsNative.ErrorMessage(code, str);
            line = str.ToString();
        }
    #endregion

    #region private methods
        [Conditional("DEBUG")]
        private static void ThrowIfError(in int result, in string functionName)
        {
            if (result != (int)SolveBoardResult.NoFault)
                throw new InvalidOperationException($"{functionName} failed with code {result}: {result.GetRCErrorMessage()}");
        }
    #endregion
}
