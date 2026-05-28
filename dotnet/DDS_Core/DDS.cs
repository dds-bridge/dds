using System.Runtime.InteropServices;
using DDS_Core;
using DDS_Core.Native;

namespace DDS_Core;

public class DDS
{
    public int SolveBoard( in Deal dl
                         , int target
                         , int solutions
                         , int mode
                         , out FutureTricks fut
                         , int threadIndex = 0)
    {
        var rc = DdsNative.SolveBoard( in dl
                                     , target
                                     , solutions
                                     , mode
                                     , out fut
                                     , threadIndex);

#if DEBUG
        if (rc != 1) // DDS return codes – check docs
            throw new InvalidOperationException($"DDS SolveBoard failed with code {rc}");
#endif
        return rc;
    }

    public SolvedBoards SolveAllBoards(Boards boards)
    {
        var rc = DdsNative.SolveAllBoards( in boards
                                         , out var solved
                                         , 0);

#if DEBUG
        if (rc != 1)
            throw new InvalidOperationException($"DDS SolveAllBoards failed with code {rc}");
#endif
        return solved;
    }

    public int SolvePBN( string pbn
                       , int target
                       , int solutions
                       , int mode
                       , out FutureTricks fut
                       , int threadIndex)
    {
        var rc = DdsNative.SolvePBN( pbn
                                   , target
                                   , solutions
                                   , mode
                                   , out  fut
                                   , threadIndex);

#if DEBUG
        if (rc != 1)
            throw new InvalidOperationException($"SolvePBN failed: {rc}");
#endif
        return rc;
    }

    public ddTableResults CalcDDTable(ddTableDeal deal)
    {
        var rc = DdsNative.CalcDDtable(ref deal, out var table);

#if DEBUG
        if (rc != 1)
            throw new InvalidOperationException($"CalcDDtable failed: {rc}");
#endif
        return table;
    }

    public parResults CalcPar(ddTableResults table)
    {
        var rc = DdsNative.CalcPar(ref table, out var pres);

#if DEBUG
        if (rc != 1)
            throw new InvalidOperationException($"CalcPar failed: {rc}");
#endif
        return pres;
    }
}
