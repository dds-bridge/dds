using DDS_Core;

namespace DDS_Core.Tests;

/// <summary>
/// Covers the context-management entry points added to the C shim by this work:
/// config-based construction, TT configuration, the resets, logging, and
/// SafeHandle-driven disposal. Between these and <see cref="SmokeTests"/>, every
/// one of the fourteen retargeted P/Invokes is exercised — so a missing
/// <c>EntryPoint</c> fails here rather than in a consumer.
/// </summary>
public class ContextLifecycleTests
{
    [Theory]
    [InlineData(TTKind.Small)]
    [InlineData(TTKind.Large)]
    public void ConstructedFromConfig_SolvesReferenceDeal(TTKind kind)
    {
        // Exercises dds_c_create_solvercontext, whose SolverConfig is unpacked
        // into scalars at the ABI boundary.
        using var ctx = new SolverContext(new SolverConfig(kind, 0, 0));

        ctx.SolveBoard(TestDeals.Reference(), -1, 1, 1, out FutureTricks fut);

        Assert.Equal(TestDeals.ExpectedTricks, fut.Score[0]);
    }

    [Fact]
    public void TtReconfiguration_LeavesContextUsable()
    {
        using var ctx = new SolverContext();
        ctx.SolveBoard(TestDeals.Reference(), -1, 1, 1, out FutureTricks _);

        ctx.ConfigureTT(TTKind.Small, 1, 2);
        ctx.ResizeTT(1, 2);
        ctx.ClearTT();

        ctx.SolveBoard(TestDeals.Reference(), -1, 1, 1, out FutureTricks fut);
        Assert.Equal(TestDeals.ExpectedTricks, fut.Score[0]);
    }

    [Fact]
    public void Resets_LeaveContextUsable()
    {
        using var ctx = new SolverContext();
        ctx.SolveBoard(TestDeals.Reference(), -1, 1, 1, out FutureTricks _);

        ctx.ResetForSolve();
        ctx.ResetBestMovesLite();

        ctx.SolveBoard(TestDeals.Reference(), -1, 1, 1, out FutureTricks fut);
        Assert.Equal(TestDeals.ExpectedTricks, fut.Score[0]);
    }

    /// <summary>
    /// A Small-TT context survives ClearTT() followed by ResetForSolve() and
    /// still solves. ClearTT() disposes the transposition table, so the
    /// following solve rebuilds one lazily from the context's configuration.
    /// This exercises the managed TT-lifecycle path; the native
    /// TransTableS::reset_memory() guard is covered by
    /// //library/tests/trans_table:trans_table.
    /// </summary>
    [Fact]
    public void SmallTt_ClearThenResetForSolve_StillSolves()
    {
        using var ctx = new SolverContext();
        ctx.SolveBoard(TestDeals.Reference(), -1, 1, 1, out FutureTricks _);

        ctx.ConfigureTT(TTKind.Small, 1, 2);
        ctx.ClearTT();
        ctx.ResetForSolve();

        ctx.SolveBoard(TestDeals.Reference(), -1, 1, 1, out FutureTricks fut);
        Assert.Equal(TestDeals.ExpectedTricks, fut.Score[0]);
    }

    [Fact]
    public void Logging_DoesNotDisturbSolving()
    {
        using var ctx = new SolverContext();

        ctx.LogAppend("DDS_Core.Tests");
        ctx.LogAppend(string.Empty);
        ctx.LogClear();

        ctx.SolveBoard(TestDeals.Reference(), -1, 1, 1, out FutureTricks fut);
        Assert.Equal(TestDeals.ExpectedTricks, fut.Score[0]);
    }

    /// <summary>
    /// Disposal must release the native context through SafeHandle without
    /// faulting, and must be safe to repeat.
    /// </summary>
    [Fact]
    public void Dispose_ReleasesHandleAndIsIdempotent()
    {
        var ctx = new SolverContext();
        ctx.SolveBoard(TestDeals.Reference(), -1, 1, 1, out FutureTricks _);

        ctx.Dispose();
        ctx.Dispose();

        Assert.True(ctx.Handle.IsClosed || ctx.Handle.IsInvalid);
    }

    /// <summary>
    /// Contexts are single-threaded, so concurrent use means one context per
    /// thread. This is the arrangement the docs prescribe; if it regressed,
    /// multi-threaded consumers would corrupt results rather than fail loudly.
    /// </summary>
    [Fact]
    public void OneContextPerThread_SolvesConcurrently()
    {
        const int threads = 4;
        var results = new int[threads];

        Parallel.For(0, threads, i =>
        {
            using var ctx = new SolverContext();
            ctx.SolveBoard(TestDeals.Reference(), -1, 1, 1, out FutureTricks fut);
            results[i] = fut.Score[0];
        });

        Assert.All(results, r => Assert.Equal(TestDeals.ExpectedTricks, r));
    }
}
