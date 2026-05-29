using System.Runtime.InteropServices;
using System.Text;
using DDS_Core;
using DDS_Core.Native;

namespace DDS_Core;

public class DDS
{
    #region ====== Configuration and Resource Management ======
        public void SetMaxThreads(int userThreads)
                                                                                                        => DdsNative.SetMaxThreads(userThreads);

        public int SetThreading(int code)
        {
            var rc = DdsNative.SetThreading( code);

            ThrowIfError(rc, nameof(SetThreading));
            return rc;
        }

        public void SetResources(int maxMemoryMB, int maxThreads)
                                                                                                            => DdsNative.SetResources(maxMemoryMB, maxThreads);

        public void FreeMemory()
                                                                                                            => DdsNative.FreeMemory();
    #endregion

    #region ====== Single Board Solving ======
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

            ThrowIfError(rc, nameof(SolveBoard));
            return rc;
        }

        public int SolveBoardPBN( DealPBN pbn
                                , int target
                                , int solutions
                                , int mode
                                , out FutureTricks fut
                                , int threadIndex)
        {
            var rc = DdsNative.SolveBoardPBN( pbn
                                            , target
                                            , solutions
                                            , mode
                                            , out  fut
                                            , threadIndex);

            ThrowIfError(rc, nameof(SolveBoardPBN));
            return rc;
        }
    #endregion

    #region ====== Multiple Board Solving ======
        public int SolveAllBoards( Boards boards
                                 , out SolvedBoards solved)
        {
            var rc = DdsNative.SolveAllBoards( in boards
                                             , out  solved);

            ThrowIfError(rc, nameof(SolveAllBoards));
            return rc;
        }

        public int SolveAllBoardsBin( in Boards bop
                                    , out SolvedBoards solved
                                    )
        {
            var rc = DdsNative.SolveAllBoardsBin( in bop
                                                , out  solved);

            ThrowIfError(rc, nameof(SolveAllBoardsBin));
            return rc;
        }

        public int SolveAllChunks( in BoardsPBN bop
                                 , out SolvedBoards solved
                                 , int chunkSize)
        {
            var rc = DdsNative.SolveAllChunks( in bop
                                             , out  solved
                                             , chunkSize);

            ThrowIfError(rc, nameof(SolveAllChunks));
            return rc;
        }

        public int SolveAllChunksBin( in Boards bop
                                    , out SolvedBoards solved
                                    , int chunkSize)
        {
            var rc = DdsNative.SolveAllChunksBin( in bop
                                                , out  solved
                                                , chunkSize);
            ThrowIfError(rc, nameof(SolveAllChunksBin));
            return rc;
        }

        public int SolveAllChunksPBN( in BoardsPBN bop
                                    , out SolvedBoards solved
                                    , int chunkSize)
        {
            var rc = DdsNative.SolveAllChunksPBN( in bop
                                                , out  solved
                                                , chunkSize);
            ThrowIfError(rc, nameof(SolveAllChunksPBN));
            return rc;
        }
    #endregion

    #region ====== Double Dummy Table Calculation ======
        public int CalcDDTable( DdTableDeal deal
                              , out DdTableResults table)
        {
            var rc = DdsNative.CalcDDtable( in deal
                                          , out table);

            ThrowIfError(rc, nameof(CalcDDTable));
            return rc;
        }

        public int CalcDDtablePBN( in DdTableDealPBN tableDealPBN
                                 , out DdTableResults table)
        {
            var rc = DdsNative.CalcDDtablePBN( in tableDealPBN
                                             , out table);

            ThrowIfError(rc, nameof(CalcDDtablePBN));
            return rc;
        }

        public int CalcAllTables( in DdTableDeals dealsp
                                , int mode
                                , int[] trumpFilter //TODO: Tjekke denne    
                                , out DdTablesRes resp
                                , out AllParResults presp)
        {
            var rc = DdsNative.CalcAllTables( in dealsp
                                            , mode
                                            , trumpFilter
                                            , out resp
                                            , out presp);

            ThrowIfError(rc, nameof(CalcAllTables));
            return rc;
        }

        public int CalcAllTablesPBN( in DdTableDealsPBN dealsp
                                   , int mode
                                   , int[] trumpFilter
                                   , out DdTablesRes resp
                                   , out AllParResults presp)
        {
            var rc = DdsNative.CalcAllTablesPBN( in dealsp
                                               , mode
                                               , trumpFilter
                                               , out resp
                                               , out presp);

            ThrowIfError(rc, nameof(CalcAllTablesPBN));
            return rc;
        }
    #endregion

    #region ====== Par Score Calculation ======
        public int Par( in DdTableResults table
                      , out ParResults pres
                      , int vulnerable)
        {
            var rc = DdsNative.Par( in table
                                  , out pres
                                  , vulnerable);

            ThrowIfError(rc, nameof(Par));
            return rc;
        }

        public int CalcPar( in DdTableDeal tableDeal
                          , int vulnerable
                          , out DdTableResults table
                          , out ParResults pres)
        {
            var rc = DdsNative.CalcPar( in tableDeal
                                      , vulnerable
                                      , out table
                                      , out pres);
            ThrowIfError(rc, nameof(CalcPar));
            return rc;
        }

        public int CalcParPBN( in DdTableDealPBN tableDealPBN
                             , out DdTableResults table
                             , int vulnerable
                             , out ParResults pres)
        {
            var rc = DdsNative.CalcParPBN( in tableDealPBN
                                         , out table
                                         , vulnerable
                                         , out pres);

            ThrowIfError(rc, nameof(CalcParPBN));
            return rc;
        }

        public int SidesPar( in DdTableResults table
                           , [Out] ParResultsDealer[] sidesRes
                           , int vulnerable)
        {
            var rc = DdsNative.SidesPar( in table
                                       , sidesRes
                                       , vulnerable);

            ThrowIfError(rc, nameof(SidesPar));
            return rc;
        }

        public int DealerPar( in DdTableResults table
                            , out ParResultsDealer pres
                            , int dealer
                            , int vulnerable)
        {
            var rc = DdsNative.DealerPar( in table
                                        , out pres
                                        , dealer
                                        , vulnerable);

            ThrowIfError(rc, nameof(DealerPar));
            return rc;
        }

        public int DealerParBin( in DdTableResults table
                               , out ParResultsMaster pres
                               , int dealer
                               , int vulnerable)
        {
            var rc = DdsNative.DealerParBin( in table
                                           , out pres
                                           , dealer
                                           , vulnerable);

            ThrowIfError(rc, nameof(DealerParBin));
            return rc;
        }

        public int SidesParBin( in DdTableResults table
                              , [Out] ParResultsMaster[] sidesRes
                              , int vulnerable)
        {
            var rc = DdsNative.SidesParBin( in table
                                          , sidesRes
                                          , vulnerable);

            ThrowIfError(rc, nameof(SidesParBin));
            return rc;
        }
    #endregion

    #region ====== Par Text Conversion ======
        public int ConvertToDealerTextFormat( in ParResultsMaster pres
                                            , StringBuilder resp)
        {
            var rc = DdsNative.ConvertToDealerTextFormat( in pres
                                                        , resp);
            ThrowIfError(rc, nameof(ConvertToDealerTextFormat));
            return rc;
        }

        public int ConvertToSidesTextFormat( in ParResultsMaster pres
                                           , out ParTextResults resp)
        {
            var rc = DdsNative.ConvertToSidesTextFormat( in pres
                                                       , out resp);
            ThrowIfError(rc, nameof(ConvertToSidesTextFormat));
            return rc;
        }
    #endregion

    #region ====== Play Analysis ======
        public int AnalysePlayBin( in Deal dl
                                 , in PlayTraceBin play
                                 , out SolvedPlay solved
                                 , int thrId)
        {
            var rc = DdsNative.AnalysePlayBin( in dl
                                             , in play
                                             , out solved
                                             , thrId);
            ThrowIfError(rc, nameof(AnalysePlayBin));
            return rc;
        }

        public int AnalysePlayPBN( in DealPBN dlPBN
                                 , in PlayTracePBN playPBN
                                 , out SolvedPlay solved
                                 , int thrId)
        {
            var rc = DdsNative.AnalysePlayPBN( in dlPBN
                                             , in playPBN
                                             , out solved
                                             , thrId);
            ThrowIfError(rc, nameof(AnalysePlayPBN));
            return rc;
        }

        public int AnalyseAllPlaysBin( in Boards bop
                                     , in PlayTracesBin plp
                                     , out SolvedPlays solved
                                     , int chunkSize)
        {
            var rc = DdsNative.AnalyseAllPlaysBin( in bop
                                                 , in plp
                                                 , out solved
                                                 , chunkSize);
            ThrowIfError(rc, nameof(AnalyseAllPlaysBin));
            return rc;
        }

        public int AnalyseAllPlaysPBN( in BoardsPBN bopPBN
                                     , in PlayTracesPBN plpPBN
                                     , out SolvedPlays solved
                                     , int chunkSize)
        {
            var rc = DdsNative.AnalyseAllPlaysPBN( in bopPBN
                                                 , in plpPBN
                                                 , out solved
                                                 , chunkSize);
            ThrowIfError(rc, nameof(AnalyseAllPlaysPBN));
            return rc;
        }
    #endregion

    #region ====== Utility Functions ======
        public void GetDDSInfo(out DDSInfo info)
        {
            DdsNative.GetDDSInfo(out info);
        }

        public void ErrorMessage( int code
                                , StringBuilder line)
        {
            DdsNative.ErrorMessage(code, line);
        }
    #endregion

    #region private methods
        private static void ThrowIfError(int result, string functionName)
        {
#if DEBUG
            if (result != (int)SolveBoardResult.NoFault)
                throw new InvalidOperationException($"{functionName} failed with code {result}: {result.GetRCErrorMessage()}");
#endif
        }
    #endregion
}
