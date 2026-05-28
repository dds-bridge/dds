using DDS_Core;
using static DDS_Core.CardRanks;

namespace DDS_Core_Demo;

internal class Program
{

    static string[]  contracts = ["spades","hearts", "diamonds", "clubs", "no trump" ];

    static void Main(string[] args)
    {
        var dds  = new DDS();
        var deal = new Deal()
                   {
                       trump            = (int)Suit.Hearts
                     , first            = 0
                     , currentTrickSuit = new int[3] {0, 0, 0 }
                     , currentTrickRank = new int[3] {0, 0, 0 }
                     , remainCards      = new uint[16]
                                          {
                                              (uint)(Jack|n6|n5|n2)
                                            , (uint)(Ace|n7|n4)
                                            , (uint)(None)
                                            , (uint)(King|Ten|n9|n6|n4|n2)
                                        //
                                            , (uint)(Ace|Ten|n9|n8|n7|n3)
                                            , (uint)(King|Jack|n5)
                                            , (uint)(Ten|n8|n2)
                                            , (uint)(n8)
                                        //
                                            , (uint)(n4)
                                            , (uint)(Queen|Ten|n9|n8|n2)
                                            , (uint)(Ace|King|Queen|n9|n3)
                                            , (uint)(Ace|Jack)
                                        //
                                            , (uint)(King|Queen)
                                            , (uint)(n6|n3)
                                            , (uint)(Jack|n7|n6|n5|n4)
                                            , (uint)(Queen|n7|n5|n3)
                                          }
                   };


        for (deal.trump = 0; deal.trump < 5; deal.trump++)
            for (deal.first = 0; deal.first < 4; deal.first++)
            {
            var rc=dds.SolveBoard(deal, -1, 1, 2, out FutureTricks fut);
            var deci = (deal.first+3)&3;
            var decl = "NESW"[deci];
            var cont = contracts[deal.trump];

            Console.WriteLine($"Tricks in {cont} for {decl}: {13 - fut.score[0]} ");
        }
            Console.WriteLine($"Press any key to continue...");


        Console.ReadKey();
    }
}
