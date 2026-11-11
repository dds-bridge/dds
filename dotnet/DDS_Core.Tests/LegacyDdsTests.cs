using DDS_Core;

namespace DDS_Core.Tests;

/// <summary>
/// Covers the obsolete <see cref="DDS"/> wrapper's own error handling, which
/// has drifted from <see cref="SolverContext"/>'s: see
/// <c>ContextLifecycleTests.SolveBoard_WithInvalidSolutions_Throws</c> for the
/// equivalent <see cref="SolverContext"/> coverage.
/// </summary>
public class LegacyDdsTests
{
    /// <summary>
    /// Failures must throw at the public surface in every build configuration,
    /// not only in DEBUG — the same contract <see cref="SolverContext"/>
    /// upholds. `solutions = 4` is out of range and the solver returns
    /// RETURN_SOLNS_WRONG_HI (-9).
    /// </summary>
    [Fact]
    public void SolveBoard_WithInvalidSolutions_Throws()
    {
#pragma warning disable CS0618 // DDS is obsolete; this test exists to cover its error handling anyway.
        var dds = new DDS();

        Assert.Throws<InvalidOperationException>(
            () => dds.SolveBoard(TestDeals.Reference(), -1, 4, 1, out FutureTricks _));
#pragma warning restore CS0618
    }
}
