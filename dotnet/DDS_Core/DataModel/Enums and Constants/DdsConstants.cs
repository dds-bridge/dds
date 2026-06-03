using System;

namespace DDS_Core
{
    /// <summary>
    /// Constants for DDS bridge analysis.
    /// </summary>
    public static class DdsConstants
    {
        /// <summary>Number of bridge strains (4 suits + no trump).</summary>
        public const int DdsStrains = 5;

        /// <summary>Number of hands (N/E/S/W).</summary>
        public const int DdsHands = 4;

        /// <summary>Number of suits (S/H/D/C).</summary>
        public const int DdsSuits = 4;

        /// <summary>No trump strain index.</summary>
        public const int DdsNoTrump = 4;

        /// <summary>Maximum number of boards in batch operations.</summary>
        public const int MaxNumberOfBoards = 200;

        /// <summary>Maximum number of DD tables.</summary>
        public const int MaxNumberOfTables = 40;
    }
}
