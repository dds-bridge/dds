using System;
using System.Runtime.InteropServices;
using DDS_Core.DataModel;
using static DDS_Core.Native.DdsNative;

namespace DDS_Core.Native;

internal static class DdsNative
{
    internal static readonly SolveBoard_t     SolveBoard;
    internal static readonly SolveAllBoards_t SolveAllBoards;
    internal static readonly SolvePBN_t       SolvePBN;
    internal static readonly CalcDDtable_t     CalcDDtable;
    internal static readonly CalcPar_t CalcPar;

    static DdsNative()
    {
        var lib = NativeLoader.Handle;

        SolveBoard     = LoadFunction<SolveBoard_t>(lib, "SolveBoard");
        SolveAllBoards = LoadFunction<SolveAllBoards_t>(lib, "SolveAllBoards");
        SolvePBN       = LoadFunction<SolvePBN_t>(lib, "SolvePBN");
        CalcDDtable = LoadFunction<CalcDDtable_t>(lib, "CalcDDtable");
        CalcPar = LoadFunction<CalcPar_t>(lib, "CalcPar");

    }

    private static T LoadFunction<T>(IntPtr lib, string name) where T : Delegate
    {
        if (!NativeLibrary.TryGetExport(lib, name, out var fn))
            throw new MissingMethodException($"DDS function not found: {name}");

        return Marshal.GetDelegateForFunctionPointer<T>(fn);
    }

    #region Define Delegate Types for DDS Functions 
        internal delegate int SolveBoard_t( in Deal dl
                                          , int target
                                          , int solutions
                                          , int mode
                                          , out FutureTricks fut
                                          , int threadIndex);

        internal delegate int SolveAllBoards_t( in Boards boards
                                              , out SolvedBoards solved
                                              , int threadIndex);

        internal delegate int SolvePBN_t( [MarshalAs(UnmanagedType.LPStr)] string pbn
                                        , int target
                                        , int solutions
                                        , int mode
                                        , out FutureTricks fut
                                        , int threadIndex);

        internal delegate int CalcDDtable_t(ref ddTableDeal deal
                                           , out ddTableResults table);

    internal delegate int CalcPar_t(ref ddTableResults table,
                                   out parResults pres);
    #endregion
}

