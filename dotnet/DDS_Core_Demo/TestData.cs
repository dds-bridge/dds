using DDS_Core;
using static DDS_Core.CardRanks;

namespace DDS_Core_Demo
{
    public static class TestData
    {
        public static string[]          pbn;
        public static uint[][][]        hands;
        public static Deal[]            deals;
        public static DealPBN[]         dealsPBN;
        public static BoardsPBN         boardsPBN;
        public static Boards            boards;
        public static DdTableDeal       ddTableDeal;
        public static DdTableDealPBN    ddTableDealPBN;
        public static DdTableDeals      ddTableDeals;
        public static DdTableDealsPBN   ddTableDealsPBN;
        public static DdTableResults    ddTableResults;
        public static ParResultsMaster  parResultsMaster;
        public static ParResultsMasters parResultsMasters;
        public static PlayTraceBin      playTraceBin;
        public static PlayTracePBN      playTracePBN;
        public static PlayTracesBin     playTracesBin;
        public static PlayTracesPBN     playTracesPBN;

        static TestData()
        {
            hands = new uint[4][][];

            hands[0] = [
                         [ (uint)(rJ|r6|r5|r2)
                         , (uint)(rA|r7|r4)
                         , (uint)None
                         , (uint)(rK|rT|r9|r6|r4|r2)
                         ]
                       , [ (uint)(rA|rT|r9|r8|r7|r3)
                         , (uint)(rK|rJ|r5)
                         , (uint)(rT|r8|r2)
                         , (uint)r8
                         ]
                       , [ (uint)r4
                         , (uint)(rQ|rT|r9|r8|r2)
                         , (uint)(rA|rK|rQ|r9|r3)
                         , (uint)(rA|rJ)
                         ]
                       , [ (uint)(rK|rQ)
                         , (uint)(r6|r3)
                         , (uint)(rJ|r7|r6|r5|r4)
                         , (uint)(rQ|r7|r5|r3)
                         ]
                       ];

            hands[1] = [
                         [ (uint)(rA|rK|r9|r6)
                         , (uint)(rK|rQ|r8)
                         , (uint)(rA|r9|r8)
                         , (uint)(rK|r6|r3)
                         ]

                       , [ (uint)(rQ|rJ|rT|r5|r4|r3|r2)
                         , (uint)(rT)
                         , (uint)(r6)
                         , (uint)(rQ|rJ|r8|r2)
                         ]

                       , [ (uint)None
                         , (uint)(rJ|r9|r7|r5|r4|r3)
                         , (uint)(rK|r7|r5|r3|r2)
                         , (uint)(r9|r4)
                         ]

                       , [ (uint)(r8|r7)
                         , (uint)(rA|r6|r2)
                         , (uint)(rQ|rJ|rT|r4)
                         , (uint)(rA|rT|r7|r5)
                         ]
                       ];

            hands[2] = [

                         [ (uint)(r7|r3)
                         , (uint)(rQ|rJ|rT)
                         , (uint)(rA|rQ|r5|r4)
                         , (uint)(rT|r7|r5|r2)
                         ]
                       , [ (uint)(rQ|rT|r6)
                         , (uint)(r8|r7|r6)
                         , (uint)(rK|rJ|r9)
                         , (uint)(rA|rQ|r8|r4)
                         ]
                       , [ (uint)r5
                         , (uint)(rA|r9|r5|r4|r3|r2)
                         , (uint)(r7|r6|r3|r2)
                         , (uint)(rK|r6)
                         ]
                       , [ (uint)(rA|rK|rJ|r9|r8|r4|r2)
                         , (uint)(rK)
                         , (uint)(rT|r8)
                         , (uint)(rJ|r9|r3)
                         ]
                       ];

            pbn = [ "N:J652.A74..KT9642 AT9873.KJ5.T82.8 4.QT982.AKQ93.AJ KQ.63.J7654.Q753"
                  , "E:QJT5432.T.6.QJ82 .J97543.K7532.94 87.A62.QJT4.AT75 AK96.KQ8.A98.K63"
                  , "N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 5.A95432.7632.K6 AKJ9842.K.T8.J93"
                  ];

            deals = new Deal[3];

            for (int i = 0; i <  deals.Length; i++)
                deals[i] = new Deal
                           {
                               Trump            = (int)Suit.Hearts
                             , First            = 0
                             , CurrentTrickSuit = new int[3] {0, 0, 0 }
                             , CurrentTrickRank = new int[3] {0, 0, 0 }
                             , RemainingCards   = TestData.hands[i]
                           };

            // dealPBN
            dealsPBN = new DealPBN[3];

            for (int i = 0; i <  dealsPBN.Length; i++)
                dealsPBN[i] = new DealPBN
                              {
                                  Trump            = (int)Suit.Hearts
                                , First            = 0
                                , CurrentTrickSuit = new int[3] {0, 0, 0 }
                                , CurrentTrickRank = new int[3] {0, 0, 0 }
                                , RemainingCards   = TestData.pbn[i]
                              };

            // boards and  boardsPBN             
            boards    = new Boards {      NumberOfBoards = 3 };
            boardsPBN = new BoardsPBN {NumberOfBoards = 3 };

            for (int i = 0; i <  boardsPBN.NumberOfBoards; i++)
            {
                boards.Deals[i]     = deals[i];
                boards.Target[i]    = -1;
                boards.Solutions[i] = 1;
                boards.Modes[i]      = 0;

                boardsPBN.Deals[i]     = dealsPBN[i];
                boardsPBN.Target[i]    = -1;
                boardsPBN.Solutions[i] = 1;
                boardsPBN.Modes[i]      = 0;
            }

            // ddTableDeal(PBN)
            ddTableDeal    = new() {   Cards = hands[0] };
            ddTableDealPBN = new() {Cards = pbn[0] };

            // ddTableDeals 
            ddTableDeals = new DdTableDeals
                           {
                               NumberOfTables = 3
                             , Deals          = new DdTableDeal[200]
                           };

            for (int i = 0; i <  ddTableDeals.NumberOfTables; i++)
                ddTableDeals.Deals[i] = new DdTableDeal { Cards = hands[i] };

            // ddTableDealsPBN 
            ddTableDealsPBN = new DdTableDealsPBN
                              {
                                  NumberOfTables = 3
                                , Deals          = new DdTableDealPBN[200]
                              };

            for (int i = 0; i <  ddTableDealsPBN.NumberOfTables; i++)
                ddTableDealsPBN.Deals[i] = new DdTableDealPBN { Cards = pbn[i] };

            // playTraceBin and PBN
            playTraceBin               = new();
            playTraceBin.NumberOfCards = 3;
            playTraceBin.Suits[0]      = 3;
            playTraceBin.Ranks[0]      = 2;

            playTraceBin.Suits[1] = 3;
            playTraceBin.Ranks[1] = 8;

            playTraceBin.Suits[2] = 3;
            playTraceBin.Ranks[2] = 14;

            var playTraceBin1           = new PlayTraceBin();
            playTraceBin1.NumberOfCards = 1;
            playTraceBin1.Suits[0]      = 0;
            playTraceBin1.Ranks[0]      = 14;

            var playTraceBin2           = new PlayTraceBin();
            playTraceBin2.NumberOfCards = 4;
            playTraceBin2.Suits[0]      = 1;
            playTraceBin2.Ranks[0]      = 12;

            playTraceBin2.Suits[1] = 1;
            playTraceBin2.Ranks[1] = 8;

            playTraceBin2.Suits[2] = 1;
            playTraceBin2.Ranks[2] = 2;

            playTraceBin2.Suits[3] = 1;
            playTraceBin2.Ranks[3]  = 13;

            playTracePBN      = new PlayTracePBN {     NumberOfPlayedCards = 3, Cards = "C2C8CA" };
            var playTracePBN2 = new PlayTracePBN {NumberOfPlayedCards = 1, Cards = "SA" };
            var playTracePBN3 = new PlayTracePBN {NumberOfPlayedCards = 4, Cards = "HQH8H2HK" };

            // playTracesBin and PBN
            playTracesBin = new();
            playTracesBin.NumberOfBoards = 3;
            playTracesBin.Plays[0]       = playTraceBin;
            playTracesBin.Plays[1]       = playTraceBin1;
            playTracesBin.Plays[2]       = playTraceBin2;

                        playTracesPBN = new();

            playTracesPBN.NumberOfBoards = 3;
            playTracesPBN.Plays[0]       = playTracePBN;
            playTracesPBN.Plays[1]       = playTracePBN2;
            playTracesPBN.Plays[2]       = playTracePBN3;
        }
    }
}
