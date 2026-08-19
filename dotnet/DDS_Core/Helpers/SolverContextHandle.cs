using System.Runtime.InteropServices;
using DDS_Core.Native;

namespace DDS_Core;
public sealed class SolverContextHandle : SafeHandle
{
    // Required by marshaling infrastructure, not intended to be used directly.
    private SolverContextHandle() : base(IntPtr.Zero, ownsHandle: true) { }

    public override bool IsInvalid => handle == IntPtr.Zero;

    protected override bool ReleaseHandle()
    {
        DdsNative.dds_destroy_solvercontext(handle);
        return true;
    }
}
