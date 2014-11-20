Bo Haglund, Bob Richardson

Rev V, 2014-10-14

Latest DLL issue with this description is available at http://www.bahnhof.se/wb758135/

# Description of the DLL functions supported in Double Dummy Problem Solver 2.7
## Callable functions
The callable functions are all preceded with `extern "C" __declspec(dllimport) int __stdcall`.  The prototypes are available in `dll.h`. 
Return codes are given at the end.

Not all functions are present in all versions of the DLL.  For historical reasons, the function names are not entirely consistent with respect to the input format.  Functions accepting binary deals will end on Bin, and those accepting PBN deals will end on PBN in the future.  At some point existing function names may be changed as well, so use the new names!

## The Basic Functions

The basic functions `SolveBoard` and `SolveBoardPBN` solve each a single hand and are thread-safe, making it possible to use them for solving several hands in parallel. The other callable functions use the SolveBoard functions either directly or indirectly.

### The Multi-Thread Double Dummy Solver Functions

The double dummy trick values for all 5 * 4 = 20 possible combinations of a hand’s trump strain and declarer hand alternatives are solved by a single call to one of the functions `CalcDDtable` and `CalcDDtablePBN`. Threads are allocated per strain.

To obtain better utilization of available threads, the double dummy (DD) tables can be grouped using one of the functions `CalcAllTables` and `CalcAllTablesPBN`.

Solving hands can be done much more quickly using one of the multi-thread alternatives for calling SolveBoard. Then a number of hands are grouped for a single call to one of the functions `SolveAllChunksBin` and `SolveAllChunksPBN`.  The hands are then solved in parallel using the available threads.

The number of threads is automatically configured by DDS, taking into account the number of processor cores and available memory.  The number of threads can be influenced using the call `SetMaxThreads`.

The call `FreeMemory` causes DDS to give up its dynamically allocated memory.

### The PAR Calculation Functions

The PAR calculation functions find the optimal contract(s) assuming open cards and optimal bidding from both sides. In very rare cases it matters which side or hand that starts the bidding, i.e. which side or hand that is first to bid its optimal contract.

Two alternatives are given:

1. The PAR scores / contracts are calculated separately for each side. In almost all cases the results will be identical for both sides, but in rare cases the result is dependent on which side that “starts the bidding”, i.e. that first finds the bid that is most beneficial for the own side. One example is when both sides can make 1 NT.
2. The dealer hand is assumed to “start the bidding”. 

The presentation of the par score and contracts are given in alternative formats.

The functions `Par`, `SidesPar` and `DealerPar` do the par calculation; their call must be preceded by a function call calculating the double dummy table values.

The functions `SidesParBin` and `DealerParBin` provide binary output of the par results, making it easy to tailor-make the output text format. Two such functions, `ConvertToSidesTextFormat` and `ConvertToDealerTextFormat`, are included as examples. 

It is possible as an option to perform par calculation in `CalcAllTables` and `CalcAllTablesPBN`. 

The par calculation is executed using a single thread. But the calculation is very fast and its duration is negligible compared to the double dummy calculation duration.

### Double Dummy Value Analyser Functions

The functions `AnalysePlayBin`, `AnalysePlayPBN`, `AnalyseAllPlaysBin` and `AnalyseAllPlaysPBN` take the played cards in a game or games and calculate and present their double dummy values.


<table>
<thead>
<tr>
<th>Function</th><th>Arguments</th><th>Format</th><th>Comment</th>
</tr>
</thead>
<tbody>
<tr>
<td rowspan="6"><code>SolveBoard</code></td><td>struct deal dl</td><td rowspan="6">Binary</td><td rowspan="6">The most basic function, solves a single hand from the beginning or from later play</td>
</tr>
<tr>
<td>int target</td>
</tr>
<tr>
<td>int solutions</td>
</tr>
<tr>
<td>int mode</td>
</tr>
<tr>
<td>struct futureTricks *futp</td>
</tr>
<tr><td>int threadIndex</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="6"><code>SolveBoardPBN</code></td><td>struct dealPBN dlPBN</td><td rowspan="6">PBN</td><td rowspan="6">As SolveBoard, but with PBN deal format.</td>
</tr>
<tr>
<td>int target</td>
</tr>
<tr>
<td>int solutions</td>
</tr>
<tr>
<td>int mode</td>
</tr>
<tr>
<td>struct futureTricks *futp</td>
</tr>
<tr>
<td>int threadIndex</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="2"><code>CalcDDtable</code></td><td>struct ddTableDeal tableDeal</td><td rowspan="2">Binary</td><td rowspan="2">Solves an initial hand for all possible declarers and denominations (up to 20 combinations.)</td>
</tr>
<tr>
<td>struct ddTableResults * tablep</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="2"><code>CalcDDtablePBN</code></td><td>struct ddTableDealPBN tableDealPBN</td><td rowspan="2">PBN</td><td rowspan="2">As CalcDDtable, but with PBN deal format.</td>
</tr>
<tr>
<td>struct ddTableResults * tablep</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="5"><code>CalcAllTables</code></td><td>struct ddTableDeals *dealsp</td><td rowspan="5">Binary</td><td rowspan="5">Solves a number of hands in parallel.  Multi-threaded.</td>
</tr>
<tr>
<td>int mode</td>
</tr>
<tr>
<td>int trumpFilter[5]</td>
</tr>
<tr>
<td>struct ddTablesRes *resp</td>
</tr>
<tr>
<td>struct allParResults *pres</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="5"><code>CalcAllTablesPBN</code></td><td>struct ddTableDealsPBN *dealsp</td><td rowspan="5">PBN</td><td rowspan="5">As CalcAllTables, but with PBN deal format.</td>
</tr>
<tr>
<td>int mode</td>
</tr>
<tr>
<td>int trumpFilter[5]</td>
</tr>
<tr>
<td>struct ddTablesRes *resp</td>
</tr>
<tr>
<td>struct allParResults *pres</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="2"><code>SolveAllBoards</code></td><td>struct boardsPBN *bop</td><td rowspan="2">PBN</td><td rowspan="2">As CalcAllTables, but with PBN deal format.</td>
</tr>
<tr>
<td>struct solvedBoards *solvedp</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="3"><code>SolveAllChunksBin</code></td><td>struct boards *bop</td><td rowspan="3">Binary</td><td rowspan="3">Solves a number of hands in parallel. Multi-threaded.</td>
</tr>
<tr>
<td>struct solvedBoards *solvedp</td>
</tr>
<tr>
<td>int chunkSize</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="3"><code>SolveAllChunks</code></td><td>struct boardsPBN *bop</td><td rowspan="3">PBN</td><td rowspan="3">Alias for SolveAllChunksPBN; don’t use!</td>
</tr>
<tr>
<td>struct solvedBoards *solvedp</td>
</tr>
<tr>
<td>int chunkSize</td>
</tr>
tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="3"><code>SolveAllChunksPBN</code></td><td>struct boardsPBN *bop</td><td rowspan="3">PBN</td><td rowspan="3">Solves a number of hands in parallel. Multi-threaded.</td>
</tr>
<tr>
<td>struct solvedBoards *solvedp</td>
</tr>
<tr>
<td>int chunkSize</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="3"><code>Par</code></td><td>struct ddTableResults *tablep</td><td rowspan="3">No format</td><td rowspan="3">Solves for the par contracts given a DD result table.</td>
</tr>
<tr>
<td>struct parResults *presp</td>
</tr>
<tr>
<td>int vulnerable</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="4"><code>DealerPar</code></td><td>struct ddTableResults *tablep</td><td rowspan="4">No format</td><td rowspan="4">Similar to Par(), but requires and uses dealer.</td>
</tr>
<tr>
<td>struct parResultsDealer *presp</td>
</tr>
<tr>
<td>int dealer</td>
</tr>
<tr>
<td>int vulnerable</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="4"><code>DealerParBin</code></td><td>struct ddTableResults *tablep</td><td rowspan="4">Binary</td><td rowspan="4">Similar to DealerPar, but with binary output.</td>
</tr>
<tr>
<td>struct parResultsMaster * presp</td>
</tr>
<tr>
<td>int dealer</td>
</tr>
<tr>
<td>int vulnerable</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="2"><code>ConvertToDealerTextFormat</code></td><td>struct parResultsMaster *pres</td><td rowspan="2">Text</td><td rowspan="2">Example of text output from DealerParBin.</td>
</tr>
<tr>
<td>char *resp</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="3"><code>SidesPar</code></td><td>struct ddTableResults *tablep</td><td rowspan="3">No format</td><td rowspan="3">Par results are given for sides with the DealerPar output format.</td>
</tr>
<tr>
<td>struct parResultsDealer *presp</td>
</tr>
<tr>
<td>int vulnerable</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="3"><code>SidesParBin</code></td><td>struct ddTableResults *tablep</td><td rowspan="3">Binary</td><td rowspan="3">Similar to SidesPar, but with binary output.</td>
</tr>
<tr>
<td>struct parResultsMaster sidesRes[2]</td>
</tr>
<tr>
<td>int vulnerable</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="2"><code>ConvertToSidesTextFormat</code></td><td>struct parResultsMaster *pres</td><td rowspan="2">Text</td><td rowspan="2">Example of text output from SidesParBin.</td>
</tr>
<tr>
<td>char *resp</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="4"><code>CalcPar</code></td><td>struct ddTableDeal tableDeal</td><td rowspan="4">Binary</td><td rowspan="4">Solves for both the DD result table and the par contracts. Is deprecated, use a CalcDDtable function plus Par() instead!</td>
</tr>
<tr>
<td>int vulnerable</td>
</tr>
<tr>
<td>struct ddTableResults *tablep</td>
</tr>
<tr>
<td>struct parResults *presp</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="4"><code>CalcParPBN</code></td><td>struct ddTableDealPBN tableDealPBN</td><td rowspan="4">PBN</td><td rowspan="4">As CalcPar, but with PBN input format. Is deprecated, use a CalcDDtable function plus Par() instead!</td>
</tr>
<tr>
<td>struct ddTableResults *tablep</td>
</tr>
<tr>
<td>int vulnerable</td>
</tr>
<tr>
<td>struct parResults *presp</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="4"><code>AnalysePlayBin</code></td><td>struct deal dl</td><td rowspan="4">Binary</td><td rowspan="4">Returns the par result after each card in a particular play sequence.</td>
</tr>
<tr>
<td>struct playTraceBin play</td>
</tr>
<tr>
<td>struct solvedPlay *solvedp</td>
</tr>
<tr>
<td>int thrId</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="4"><code>AnalysePlayPBN</code></td><td>struct dealPBN dlPBN</td><td rowspan="4">PBN</td><td rowspan="4">As AnalysePlayBin, but with PBN deal format.</td>
</tr>
<tr>
<td>struct playTracePBN playPBN</td>
</tr>
<tr>
<td>struct solvedPlay *solvedp</td>
</tr>
<tr>
<td>int thrId</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="4"><code>AnalyseAllPlaysBin</code></td><td>struct boards *bop</td><td rowspan="4">Binary</td><td rowspan="4">Solves a number of hands with play sequences in parallel.  Multi-threaded.</td>
</tr>
<tr>
<td>struct playTracesBin *plp</td>
</tr>
<tr>
<td>struct solvedPlays *solvedp</td>
</tr>
<tr>
<td>int chunkSize</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="4"><code>AnalyseAllPlaysPBN</code></td><td>struct boardsPBN *bopPBN</td><td rowspan="4">PBN</td><td rowspan="4">As AnalyseAllPlaysBin, but with PBN deal format.</td>
</tr>
<tr>
<td>struct playTracesPBN *plpPBN</td>
</tr>
<tr>
<td>struct solvedPlays *solvedp</td>
</tr>
<tr>
<td>int chunkSize</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td><code>SetMaxThreads</code></td><td>int userThreads</td><td>PBN</td><td>Used at initial start and can also be called with a request for allocating memory for a specified number of threads.</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td><code>FreeMemory</code></td><td>void</td><td>PBN</td><td>Frees DDS allocated dynamical memory.</td>
</tr>
</tbody>
</table>
