using System;
using System.IO;
using System.Runtime.InteropServices;

namespace DDS_Core.Native;

internal static class NativeLoader
{
    private static readonly IntPtr _handle;

    static NativeLoader()
    {
        var fileName = GetLibraryFileName();
        var fullPath = ResolveLibraryPath(fileName);

        if (!NativeLibrary.TryLoad(fullPath, out _handle))
            throw new DllNotFoundException($"Could not load native DDS library: {fullPath}");
    }

    public static IntPtr Handle => _handle;

    private static string GetLibraryFileName()
    {
        if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
            return "dds.dll";

        if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
            return "libdds.so";

        if (RuntimeInformation.IsOSPlatform(OSPlatform.OSX))
            return "libdds.dylib";

        throw new PlatformNotSupportedException();
    }

    private static string ResolveLibraryPath(string fileName)
    {
        var baseDir = AppContext.BaseDirectory;

        string[] search =
        {
            baseDir,
            Path.Combine(baseDir, "runtimes", GetRid(), "native"),
            Path.Combine(baseDir, "native")
        };

        foreach (var dir in search)
        {
            var path = Path.Combine(dir, fileName);
            if (File.Exists(path))
                return path;
        }

        return fileName; // fallback: let OS search PATH
    }

    private static string GetRid()
    {
        if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
            return "win-x64";

        if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
            return "linux-x64";

        if (RuntimeInformation.IsOSPlatform(OSPlatform.OSX))
            return RuntimeInformation.OSArchitecture == Architecture.Arm64
                ? "osx-arm64"
                : "osx-x64";

        throw new PlatformNotSupportedException();
    }
}
