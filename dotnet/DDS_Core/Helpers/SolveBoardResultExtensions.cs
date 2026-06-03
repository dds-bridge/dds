namespace DDS_Core.Helpers
{
    public static class SolveBoardResultExtensions
    {
        public static string GetRCErrorMessage(this int result)
        {
            return GetRcErrorMessage((SolveBoardResult)result);
        }

        public static string GetRcErrorMessage(this SolveBoardResult result)
            => result switch
             {
             SolveBoardResult.NoFault          => "No fault",
             SolveBoardResult.UnknownFault     => "General error",
             SolveBoardResult.ZeroCards        => "Zero cards",
             SolveBoardResult.TargetTooHigh    => "Target exceeds number of tricks",
             SolveBoardResult.DuplicateCards   => "Cards duplicated",
             SolveBoardResult.TargetWrongLo    => "Target is less than -1",
             SolveBoardResult.TargetWrongHi    => "Target is higher than 13",
             SolveBoardResult.SolutionsWrongLo => "Solutions parameter is less than 1",
             SolveBoardResult.SolutionsWrongHi => "Solutions parameter is higher than 3",
             SolveBoardResult.TooManyCards     => "Too many cards",
             SolveBoardResult.SuitOrRank       => "currentTrickSuit or currentTrickRank has wrong data",
             SolveBoardResult.PlayedCard       => "Played card also remains in a hand",
             SolveBoardResult.CardCount        => "Wrong number of remaining cards in a hand",
             SolveBoardResult.ThreadIndex      => "Thread index is not 0 .. maximum",
             SolveBoardResult.ModeWrongLo      => "Mode parameter is less than 0",
             SolveBoardResult.ModeWrongHi      => "Mode parameter is higher than 2",
             SolveBoardResult.TrumpWrong       => "Trump is not in 0 .. 4",
             SolveBoardResult.FirstWrong       => "First is not in 0 .. 2",
             SolveBoardResult.PlayFault        => "AnalysePlay input error",
             SolveBoardResult.PbnFault         => "PBN string error",
             SolveBoardResult.TooManyBoards    => "Too many Boards requested",
             SolveBoardResult.ThreadCreate     => "Could not create threads",
             SolveBoardResult.ThreadWait       => "Something failed waiting for thread to end",
             SolveBoardResult.ThreadMissing    => "Tried to set a multi-threading system that is not present",
             _                                 => "Unknown error"
             };
    }
}
