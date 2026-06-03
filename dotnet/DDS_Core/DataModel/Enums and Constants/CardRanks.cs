namespace DDS_Core;

public enum CardRanks : uint
{
      None = 0
    , rA = 0x00000001 <<14
    , rK = 0x00000001 <<13
    , rQ = 0x00000001 <<12
    , rJ = 0x00000001 <<11
    , rT = 0x00000001 <<10
    , r9 = 0x00000001 <<9
    , r8 = 0x00000001 <<8
    , r7 = 0x00000001 <<7
    , r6 = 0x00000001 <<6
    , r5 = 0x00000001 <<5
    , r4 = 0x00000001 <<4
    , r3 = 0x00000001 <<3
    , r2 = 0x00000001 <<2
    , All = 0x00007FFC       // All ranks (except None)
};

