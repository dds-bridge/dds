using System.Reflection;
using System.Runtime.InteropServices;

namespace DDS_Core.Native;

/// <summary>
/// Resolves the native DDS library for this assembly's P/Invokes.
/// </summary>
/// <remarks>
/// <para>
/// By default the runtime's own probing applies, which is what a NuGet package
/// laying the library out under <c>runtimes/&lt;rid&gt;/native</c> relies on.
/// Setting the <c>DDS_LIBRARY_PATH</c> environment variable to the full path of
/// a library file overrides that, which is how tests and development builds bind
/// against a freshly-built <c>bazel-bin/jni/libdds.dylib</c> without installing
/// anything. It is the .NET counterpart of the JVM binding's
/// <c>-Ddds.library.path</c>.
/// </para>
/// <para>
/// If <c>DDS_LIBRARY_PATH</c> is set but the library cannot be loaded, the
/// failure is surfaced with the attempted path rather than silently falling back
/// to probing: a typo in a test script should not present later as a missing
/// entry point.
/// </para>
/// </remarks>
internal static class DdsNativeResolver
{
    /// <summary>Environment variable holding an explicit path to the native library.</summary>
    internal const string LibraryPathVariable = "DDS_LIBRARY_PATH";

    /// <summary>
    /// Registers the resolver. Called from <see cref="DdsNative"/>'s static
    /// constructor, which the runtime guarantees runs before that type's first
    /// P/Invoke — so no explicit setup call is needed from consumers. A module
    /// initializer would also work but runs eagerly at load time, which is both
    /// more surprising in a library and flagged by CA2255.
    /// </summary>
    internal static void Register()
        => NativeLibrary.SetDllImportResolver(typeof(DdsNative).Assembly, Resolve);

    private static IntPtr Resolve(string libraryName, Assembly assembly, DllImportSearchPath? searchPath)
    {
        // Never intercept imports belonging to anything but the DDS library.
        if (!string.Equals(libraryName, "dds", StringComparison.Ordinal))
            return IntPtr.Zero;

        var explicitPath = Environment.GetEnvironmentVariable(LibraryPathVariable);
        if (string.IsNullOrWhiteSpace(explicitPath))
            return IntPtr.Zero;   // Fall back to the runtime's default probing.

        try
        {
            return NativeLibrary.Load(explicitPath);
        }
        catch (Exception ex)
        {
            throw new DllNotFoundException(
                $"{LibraryPathVariable} is set to '{explicitPath}', but the native DDS "
                + "library could not be loaded from there.", ex);
        }
    }
}
