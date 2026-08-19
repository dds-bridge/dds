using System;
using System.Runtime.InteropServices;

namespace DDS_Core;

/// <summary>
/// DDS library information and configuration details.
/// 
/// Contains version, platform, compiler, threading, and resource information.
/// </summary>
[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
public struct DdsInfo
{
    /// <summary>Major version number (e.g., 3 for 3.1.0).</summary>
    public int Major;

    /// <summary>Minor version number.</summary>
    public int Minor;

    /// <summary>Patch version number.</summary>
    public int Patch;

    /// <summary>Version string (e.g., "3.1.0"), max 10 characters.</summary>
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 10)]
    public readonly string VersionString;

    /// <summary>Platform (0 = unknown, 1 = Windows, 2 = Cygwin, 3 = Linux, 4 = macOS).</summary>
    public int System;

    /// <summary>Bit width (32 or 64).</summary>
    public int NumberOfBits;

    /// <summary>Compiler (0 = unknown, 1 = MSVC, 2 = mingw, 3 = GCC, 4 = clang).</summary>
    public int Compiler;

    /// <summary>Constructor type (0 = none, 1 = DllMain, 2 = Unix-style).</summary>
    public int constructor;

    /// <summary>Number of processor cores available.</summary>
    public int NumberOfCores;

    /// <summary>Threading system (0 = none, 1 = Windows, 2 = OpenMP, 3 = GCD, 4 = Boost, 5 = STL, 6 = TBB, etc.).</summary>
    public int Threading;

    /// <summary>Actual number of threads configured.</summary>
    public int NumberOfThreads;

    /// <summary>Thread memory sizes configuration string (max 128 characters).</summary>
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
    public readonly string ThreadSizes;

    /// <summary>Detailed system information string (max 1024 characters).</summary>
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 1024)]
    public readonly string SystemString;


}
