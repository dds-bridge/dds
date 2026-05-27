using System.Runtime.InteropServices;

[StructLayout(LayoutKind.Sequential, Pack = 1)]
public struct parResults
{
    public int score_NS;
    public int score_EW;

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 20)]
    public int[] parScore;
}
