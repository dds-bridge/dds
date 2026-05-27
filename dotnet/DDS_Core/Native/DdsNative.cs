using System;
using System.Runtime.InteropServices;
using DDS_Core.DataModel;

namespace DDS_Core.Native;

internal static class DdsNative
{
    [DllImport("dds_native")]
    public static extern int SolveBoard(in Deal dl,
                                         int target,
                                         int solutions,
                                         int mode,
                                         out FutureTricks fut,
                                         int threadIndex);

    [DllImport("dds_native")]
    public static extern int SolveAllBoards(in Boards boards,
                                            out SolvedBoards solved,
                                             int threadIndex);

    [DllImport("dds_native")]
    public static extern int SolvePBN([MarshalAs(UnmanagedType.LPStr)] string pbn,
                                       int target,
                                       int solutions,
                                       int mode,
                                       out FutureTricks fut,
                                       int threadIndex);

    [DllImport("dds_native")]
    public static extern int CalcDDtable(ref ddTableDeal deal,
                                          out ddTableResults table);

    [DllImport("dds_native")]
    public static extern int CalcPar(ref ddTableResults table,
                                      out parResults pres);

    internal static int SolveBoard_Wrapper(in Deal dl,
                                           int target,
                                           int solutions,
                                           int mode,
                                           out FutureTricks fut,
                                           int threadIndex)
        => SolveBoard(in dl, target, solutions, mode, out fut, threadIndex);

    internal static int SolveAllBoards_Wrapper(in Boards boards,
                                               out SolvedBoards solved,
                                               int threadIndex)
        => SolveAllBoards(in boards, out solved, threadIndex);

    internal static int SolvePBN_Wrapper(string pbn,
                                         int target,
                                         int solutions,
                                         int mode,
                                         out FutureTricks fut,
                                         int threadIndex)
        => SolvePBN(pbn, target, solutions, mode, out fut, threadIndex);

    internal static int CalcDDtable_Wrapper(ref ddTableDeal deal,
                                            out ddTableResults table)
        => CalcDDtable(ref deal, out table);

    internal static int CalcPar_Wrapper(ref ddTableResults table,
                                        out parResults pres)
        => CalcPar(ref table, out pres);
}

