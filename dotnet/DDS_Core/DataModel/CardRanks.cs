namespace DDS_Core
{
    public enum CardRanks : uint
    {
None
        , Ace = 0x00000001 <<14
        , King = 0x00000001 <<13
        , Queen = 0x00000001 <<12
        , Jack = 0x00000001 <<11
        , Ten = 0x00000001 <<10
        , n9 = 0x00000001 <<9
        , n8 = 0x00000001 <<8
        , n7 = 0x00000001 <<7
        , n6 = 0x00000001 <<6
        , n5 = 0x00000001 <<5
        , n4 = 0x00000001 <<4
        , n3 = 0x00000001 <<3
        , n2 = 0x00000001 <<2

        , All = 0x00007FFC       // All ranks (except None)
    };
}
