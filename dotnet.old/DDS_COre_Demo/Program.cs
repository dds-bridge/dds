using DDSCore;

namespace DotNetDemo;

internal class Program
{
    static void Main(string[] args)
    {
        var dds  = new DDSCore.dds();
        var deal = new DDSCore.ManagedDeal();
        var cfg  = new DDSCore.ManagedSolverConfig();
        var ctx  = new ManagedSolverContext(cfg);
        var fut  = new DDSCore.ManagedFutureTricks();

        var rc = dds.SolveBoard(ctx, deal, 10, 1, 0, fut);

        Console.WriteLine("Hello, World!");
    }
}
