namespace DdTableForDeal;

internal static class Program
{
    private static int Main(string[] args)
    {
        var argv = new string[args.Length + 1];
        argv[0] = Environment.GetCommandLineArgs()[0];
        args.CopyTo(argv, 1);

        return DdTableForDealApp.Run(
            argv,
            Console.Out,
            Console.Error,
            stdinIsTty: !Console.IsInputRedirected,
            stdin: Console.In);
    }
}
