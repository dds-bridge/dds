using System.Runtime.InteropServices;

namespace DDS_Core;

[StructLayout(LayoutKind.Sequential)]
public struct SolverConfig
{
    public TTKind TTKind;
    public int    DefaultMemoryMB;
    public int    MaximumMemoryMB;

    public SolverConfig()
    {
        TTKind = TTKind.Large;
    }

    public SolverConfig(TTKind tTKind, int defaultMemoryMB, int maximumMemoryMB)
    {
        TTKind          = tTKind;
        DefaultMemoryMB = defaultMemoryMB;
        MaximumMemoryMB = maximumMemoryMB;
    }
}
