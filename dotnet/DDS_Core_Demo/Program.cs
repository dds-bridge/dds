using DDS_Core;
using static DDS_Core.CardRanks;

namespace DDS_Core_Demo;

internal class Program
{
    private static string[] contracts = ["spades","hearts", "diamonds", "clubs", "no trump" ];

    static void Main(string[] args)
    {
        uint[][] hands = [ new uint[]
                           {
                               (uint)(rJ|r6|r5|r2)
                             , (uint)(rA|r7|r4)
                             , (uint)None
                             , (uint)(rK|rT|r9|r6|r4|r2)
                           }

                         , new uint[]
                           {
                               (uint)(rA|rT|r9|r8|r7|r3)
                             , (uint)(rK|rJ|r5)
                             , (uint)(rT|r8|r2)
                             , (uint)r8
                           }

                         , new uint[]
                           {
                               (uint)r4
                             , (uint)(rQ|rT|r9|r8|r2)
                             , (uint)(rA|rK|rQ|r9|r3)
                             , (uint)(rA|rJ)
                           }

                         , new uint[]
                           {
                               (uint)(rK|rQ)
                             , (uint)(r6|r3)
                             , (uint)(rJ|r7|r6|r5|r4)
                             , (uint)(rQ|r7|r5|r3)
                           }

                         ];

        var dds  = new DDS();
        var deal = new Deal
                   {
                       trump            = (int)Suit.Hearts
                     , first            = 0
                     , currentTrickSuit = new int[3] {0, 0, 0 }
                     , currentTrickRank = new int[3] {0, 0, 0 }
                     , remainCards      = hands      
                   };

        // SolveBoard: Loop through all possible contracts and first players
        for (deal.trump = 0; deal.trump <  5; deal.trump++)
            for (deal.first = 0; deal.first <  4; deal.first++)
            {
                var rc   =dds.SolveBoard(deal, -1, 1, 0, out FutureTricks fut);
                var deci = (deal.first+3)&3;
                var decl = "NESW"[deci];
                var cont = contracts[deal.trump];

                Console.WriteLine($"Tricks in {cont} for {decl}: {13 - fut.score[0]} ");
            }

        Console.WriteLine($"Press any key to continue...");
        Console.ReadKey();
        return;
    }
}
