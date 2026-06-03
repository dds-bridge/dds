using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection.Metadata;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
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
