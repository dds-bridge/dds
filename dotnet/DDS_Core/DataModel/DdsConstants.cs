using System;

namespace DDS_Core
{
    /// <summary>
    /// Constants for DDS bridge analysis.
    /// </summary>
    public static class DdsConstants
    {
        /// <summary>Number of bridge strains (4 suits + no trump).</summary>
        public const int DDS_STRAINS = 5;

        /// <summary>Number of hands (N/E/S/W).</summary>
        public const int DDS_HANDS = 4;

        /// <summary>Number of suits (S/H/D/C).</summary>
        public const int DDS_SUITS = 4;

        /// <summary>No trump strain index.</summary>
        public const int DDS_NOTRUMP = 4;

        /// <summary>Maximum number of boards in batch operations.</summary>
        public const int MAXNOOFBOARDS = 200;

        /// <summary>Maximum number of DD tables.</summary>
        public const int MAXNOOFTABLES = 40;
    }
}
