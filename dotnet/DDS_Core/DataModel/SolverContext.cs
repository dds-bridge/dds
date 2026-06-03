using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using DDS_Core.Helpers;
using DDS_Core.Native;

namespace DDS_Core;

public sealed class SolverContext : IDisposable
{
    public SolverContextHandle Handle { get; }

    #region Constructors and destructors
        public SolverContext()
        {
            Handle = DdsNative.dds_create_solvercontext_default()
                  ?? throw new InvalidOperationException("Failed to create SolverContext.");
        }

        public SolverContext(SolverConfig config)
        {
            Handle = DdsNative.dds_create_solvercontext(config)
                  ?? throw new InvalidOperationException("Failed to create SolverContext.");
        }

        public void Dispose()
        {
            Handle?.Dispose();
            GC.SuppressFinalize(this);
        }
    #endregion

    #region TT Management
        public void ConfigureTT(TTKind kind, int defaultMb, int maxMb)
                                => DdsNative.dds_configure_tt(Handle, kind, defaultMb, maxMb);

        public void ResizeTT(int defaultMb, int maxMb)
                                => DdsNative.dds_resize_tt(Handle, defaultMb, maxMb);

        public void ClearTT()
                                => DdsNative.dds_clear_tt(Handle);

        public void ResetForSolve()
                                => DdsNative.dds_reset_for_solve(Handle);

        public void ResetBestMovesLite()
                                => DdsNative.dds_reset_best_moves_lite(Handle);
    #endregion

    #region Solving
        public int SolveBoard( in Deal dl
                             , int target
                             , int solutions
                             , int mode
                             , out FutureTricks fut)
        {
            var rc = DdsNative.dds_solve_board( Handle
                                              , dl
                                              , target
                                              , solutions
                                              , mode
                                              , out fut);

            ThrowIfError(rc, nameof(SolveBoard));
            return rc;
        }

        public int CalcDdTable( in DdTableDeal table_deal
                              , out DdTableResults table_results)
        {
            var rc = DdsNative.calc_dd_table( Handle
                                            , in table_deal
                                            , out table_results);

            ThrowIfError(rc, nameof(SolveBoard));
            return rc;
        }

        public int CalcDdTablePBN( in DdTableDealPBN table_deal_pbn
                                 , out DdTableResults table_results)
        {
            var rc = DdsNative.calc_dd_table_pbn( Handle
                                                , in table_deal_pbn
                                                , out table_results);

            ThrowIfError(rc, nameof(CalcDdTablePBN));
            return rc;
        }

    /// <summary>
/// Calculates par score and contracts for a deal table.
/// </summary>
/// <remarks>
/// <para>
/// Computes the double dummy table for the given deal, then calculates par score
/// and contracts based on vulnerability.
/// </para>
/// <para>
/// This function is equivalent to calling <c>CalcDDtable</c> followed by
/// <c>Par</c> in the legacy C API.
/// </para>
/// </remarks>
/// <param name="table_deal">
/// Deal represented as card holdings for each hand.
/// </param>
/// <param name="vulnerable">
/// Vulnerability (0=None, 1=Both, 2=NS, 3=EW).
/// </param>
/// <param name="table_results">
/// Output: double dummy table results.
/// </param>
/// <param name="par_results">
/// Output: par score and contract strings.
/// </param>
/// <returns>
/// Error code (<c>RETURN_NO_FAULT</c> on success).
/// </returns>

        public int CalcPar( in DdTableDeal table_deal
                          , int vulnerable
                          , out DdTableResults table_results
                          , out ParResults par_results)
        {
            var rc = DdsNative.calc_par( Handle
                                       , in table_deal
                                       , vulnerable
                                       , out table_results
                                       , out par_results);
            ThrowIfError(rc, nameof(CalcPar));
            return rc;
        }
    #endregion

    #region Logging
        public void LogAppend(string message)
                                        => DdsNative.dds_log_append(Handle, message);

        public void LogClear()
                                => DdsNative.dds_log_clear(Handle);
    #endregion

    #region private methods
        private static void ThrowIfError(int result, string functionName)
        {
#if DEBUG
            if (result != (int)SolveBoardResult.NoFault)
                throw new InvalidOperationException($"{functionName} failed with code {result}: {result.GetRCErrorMessage()}");
#endif
        }
    #endregion
}

