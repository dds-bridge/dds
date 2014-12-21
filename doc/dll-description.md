Bo Haglund, Soren Hein, Bob Richardson

Rev X, 2014-11-16

Latest DLL issue with this description is available at http://www.bahnhof.se/wb758135/

# Description of the DLL functions supported in Double Dummy Problem Solver 2.8
## Callable functions
The callable functions are all preceded with `extern "C" __declspec(dllimport) int __stdcall`.  The prototypes are available in `dll.h`, in the include directory.

Return codes are given at the end.

Not all functions are present in all versions of the DLL.  For historical reasons, the function names are not entirely consistent with respect to the input format.  Functions accepting binary deals will end on Bin, and those accepting PBN deals will end on PBN in the future.  At some point existing function names may be changed as well, so use the new names!

## The Basic Functions

The basic functions `SolveBoard` and `SolveBoardPBN` each solve  a single hand and are thread-safe, making it possible to use them for solving several hands in parallel. The other callable functions use the SolveBoard functions either directly or indirectly.

### The Multi-Thread Double Dummy Solver Functions

The double dummy trick values for all 5 \* 4 = 20 possible combinations of a hand’s trump strain and declarer hand alternatives are solved by a single call to one of the functions `CalcDDtable` and `CalcDDtablePBN`. Threads are allocated per strain. in order to save computations.

To obtain better utilization of available threads, the double dummy (DD) tables can be grouped using one of the functions `CalcAllTables` and `CalcAllTablesPBN`.

Solving hands can be done much more quickly using one of the multi-thread alternatives for calling SolveBoard. Then a number of hands are grouped for a single call to one of the functions `SolveAllBorads`, `SolveAllChunksBin` and `SolveAllChunksPBN`.  The hands are then solved in parallel using the available threads.

The number of threads is automatically configured by DDS on Windows, taking into account the number of processor cores and available memory.  The number of threads can be influenced using by calling `SetMaxThreads`. This function should probably always be called on Linux/Mac, with a zero argument for auto-configuration.

Calling `FreeMemory` causes DDS to give up its dynamically allocated memory.

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
<td>struct futureTricks \*futp</td>
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
<td>struct futureTricks \*futp</td>
</tr>
<tr>
<td>int threadIndex</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="2"><code>CalcDDtable</code></td><td>struct ddTableDeal tableDeal</td><td rowspan="2">Binary</td><td rowspan="2">Solves an initial hand for all possible declarers and denominations (up to 20 combinations.)</td>
</tr>
<tr>
<td>struct ddTableResults \* tablep</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="2"><code>CalcDDtablePBN</code></td><td>struct ddTableDealPBN tableDealPBN</td><td rowspan="2">PBN</td><td rowspan="2">As CalcDDtable, but with PBN deal format.</td>
</tr>
<tr>
<td>struct ddTableResults \* tablep</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="5"><code>CalcAllTables</code></td><td>struct ddTableDeals \*dealsp</td><td rowspan="5">Binary</td><td rowspan="5">Solves a number of hands in parallel.  Multi-threaded.</td>
</tr>
<tr>
<td>int mode</td>
</tr>
<tr>
<td>int trumpFilter[5]</td>
</tr>
<tr>
<td>struct ddTablesRes \*resp</td>
</tr>
<tr>
<td>struct allParResults \*pres</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="5"><code>CalcAllTablesPBN</code></td><td>struct ddTableDealsPBN \*dealsp</td><td rowspan="5">PBN</td><td rowspan="5">As CalcAllTables, but with PBN deal format.</td>
</tr>
<tr>
<td>int mode</td>
</tr>
<tr>
<td>int trumpFilter[5]</td>
</tr>
<tr>
<td>struct ddTablesRes \*resp</td>
</tr>
<tr>
<td>struct allParResults \*pres</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="2"><code>SolveAllBoards</code></td><td>struct boardsPBN \*bop</td><td rowspan="2">PBN</td><td rowspan="2">Consider using this instead of the next 3 “Chunk” functions”!</td>
</tr>
<tr>
<td>struct solvedBoards \*solvedp</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="3"><code>SolveAllChunksBin</code></td><td>struct boards \*bop</td><td rowspan="3">Binary</td><td rowspan="3">Solves a number of hands in parallel. Multi-threaded.</td>
</tr>
<tr>
<td>struct solvedBoards \*solvedp</td>
</tr>
<tr>
<td>int chunkSize</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="3"><code>SolveAllChunks</code></td><td>struct boardsPBN \*bop</td><td rowspan="3">PBN</td><td rowspan="3">Alias for SolveAllChunksPBN; don’t use!</td>
</tr>
<tr>
<td>struct solvedBoards \*solvedp</td>
</tr>
<tr>
<td>int chunkSize</td>
</tr>
<td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="3"><code>SolveAllChunksPBN</code></td><td>struct boardsPBN \*bop</td><td rowspan="3">PBN</td><td rowspan="3">Solves a number of hands in parallel. Multi-threaded.</td>
</tr>
<tr>
<td>struct solvedBoards \*solvedp</td>
</tr>
<tr>
<td>int chunkSize</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="3"><code>Par</code></td><td>struct ddTableResults \*tablep</td><td rowspan="3">No format</td><td rowspan="3">Solves for the par contracts given a DD result table.</td>
</tr>
<tr>
<td>struct parResults \*presp</td>
</tr>
<tr>
<td>int vulnerable</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="4"><code>DealerPar</code></td><td>struct ddTableResults \*tablep</td><td rowspan="4">No format</td><td rowspan="4">Similar to Par(), but requires and uses knowledge
of the dealer.</td>
</tr>
<tr>
<td>struct parResultsDealer \*presp</td>
</tr>
<tr>
<td>int dealer</td>
</tr>
<tr>
<td>int vulnerable</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="4"><code>DealerParBin</code></td><td>struct ddTableResults \*tablep</td><td rowspan="4">Binary</td><td rowspan="4">Similar to DealerPar, but with binary output.</td>
</tr>
<tr>
<td>struct parResultsMaster \* presp</td>
</tr>
<tr>
<td>int dealer</td>
</tr>
<tr>
<td>int vulnerable</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="2"><code>ConvertToDealerTextFormat</code></td><td>struct parResultsMaster \*pres</td><td rowspan="2">Text</td><td rowspan="2">Example of text output from DealerParBin.</td>
</tr>
<tr>
<td>char \*resp</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="3"><code>SidesPar</code></td><td>struct ddTableResults \*tablep</td><td rowspan="3">No format</td><td rowspan="3">Par results are given for sides with the DealerPar output format.</td>
</tr>
<tr>
<td>struct parResultsDealer \*presp</td>
</tr>
<tr>
<td>int vulnerable</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="3"><code>SidesParBin</code></td><td>struct ddTableResults \*tablep</td><td rowspan="3">Binary</td><td rowspan="3">Similar to SidesPar, but with binary output.</td>
</tr>
<tr>
<td>struct parResultsMaster sidesRes[2]</td>
</tr>
<tr>
<td>int vulnerable</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="2"><code>ConvertToSidesTextFormat</code></td><td>struct parResultsMaster \*pres</td><td rowspan="2">Text</td><td rowspan="2">Example of text output from SidesParBin.</td>
</tr>
<tr>
<td>char \*resp</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="4"><code>CalcPar</code></td><td>struct ddTableDeal tableDeal</td><td rowspan="4">Binary</td><td rowspan="4">Solves for both the DD result table and the par contracts. Is deprecated, use a CalcDDtable function plus Par() instead!</td>
</tr>
<tr>
<td>int vulnerable</td>
</tr>
<tr>
<td>struct ddTableResults \*tablep</td>
</tr>
<tr>
<td>struct parResults \*presp</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="4"><code>CalcParPBN</code></td><td>struct ddTableDealPBN tableDealPBN</td><td rowspan="4">PBN</td><td rowspan="4">As CalcPar, but with PBN input format. Is deprecated, use a CalcDDtable function plus Par() instead!</td>
</tr>
<tr>
<td>struct ddTableResults \*tablep</td>
</tr>
<tr>
<td>int vulnerable</td>
</tr>
<tr>
<td>struct parResults \*presp</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="4"><code>AnalysePlayBin</code></td><td>struct deal dl</td><td rowspan="4">Binary</td><td rowspan="4">Returns the par result after each card in a particular play sequence.</td>
</tr>
<tr>
<td>struct playTraceBin play</td>
</tr>
<tr>
<td>struct solvedPlay \*solvedp</td>
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
<td>struct solvedPlay \*solvedp</td>
</tr>
<tr>
<td>int thrId</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="4"><code>AnalyseAllPlaysBin</code></td><td>struct boards \*bop</td><td rowspan="4">Binary</td><td rowspan="4">Solves a number of hands with play sequences in parallel.  Multi-threaded.</td>
</tr>
<tr>
<td>struct playTracesBin \*plp</td>
</tr>
<tr>
<td>struct solvedPlays \*solvedp</td>
</tr>
<tr>
<td>int chunkSize</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td rowspan="4"><code>AnalyseAllPlaysPBN</code></td><td>struct boardsPBN \*bopPBN</td><td rowspan="4">PBN</td><td rowspan="4">As AnalyseAllPlaysBin, but with PBN deal format.</td>
</tr>
<tr>
<td>struct playTracesPBN \*plpPBN</td>
</tr>
<tr>
<td>struct solvedPlays \*solvedp</td>
</tr>
<tr>
<td>int chunkSize</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td><code>SetMaxThreads</code></td><td>int userThreads</td><td>PBN</td><td>Used at initial start and can also be called with a request for allocating memory for a specified number of threads. Is apparently¸mandatory on Linux and Mac (optional on Windows)</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td><code>FreeMemory</code></td><td>void</td><td>&nbsp;</td><td>Frees DDS allocated dynamical memory.</td>
</tr>
<tr><td colspan="4">&nbsp;</td></tr>
<tr>
<td roespan="2">ErrorMessage</td><td>int code</td><td>&nbsp;</td><td roenspan="2">Turns a return code into an error message string.</td>
</tr>
<tr>
<td>
</tbody>
</table>

## Data structure
Common encodings are as follows

<table>
<thead>
<tr>
<th>Encoding</th><th>Element</th><th>Value</th>
</tr>
</thead>
<tbody>
<tr>
<td rowspan="5">Suit</td><td>Spades</td><td>0</td>
</tr>
<tr><td>Hearts</td><td>1</td>
</tr>
<tr>
<td>Diamonds</td><td>2</td>
</tr>
<tr>
<td>Clubs</td><td>3</td>
</tr>
<tr>
<td>NT</td><td>4</td>
</tr>
<tr><td colspan="3">&nbsp;</td></tr>
<td rowspan="4">Hand</td><td>North</td><td>0</td>
</tr>
<tr><td>East</td><td>1</td>
</tr>
<tr>
<td>South</td><td>2</td>
</tr>
<tr>
<td>West</td><td>3</td>
</tr>
<tr><td colspan="2">&nbsp;</td></tr>
<td rowspan="2">Side</td><td>N-S</td><td>0</td>
</tr>
<tr><td>E-W</td><td>1</td>
</tr>
<tr><td colspan="3">&nbsp;</td></tr>
<td rowspan="4">Card</td><td>Bit 2</td><td>Rank of deuce</td>
</tr>
<tr><td>...</td><td>&nbsp;</td>
</tr>
<tr>
<td>Bit 13</td><td>Rank of king</td>
</tr>
<tr>
<td>Bit 14</td><td>Rank of ace</td>
</tr>
<tr><td colspan="3">&nbsp;</td></tr>
<td>Holding</td><td colspan="2">A value of 16388 = 16384 + 4 is the encoding for the holding “A2” (ace and deuce).<br />The two lowest bits are always zero.</td>
</tr>
<tr><td colspan="3">&nbsp;</td></tr>
<td>PBN</td><td colspan="2">Example:<br />W:T5.K4.652.A98542 K6.QJT976.QT7.Q6 432.A.AKJ93.JT73 AQJ987.8532.84.K</td>
</tr>
</tbody>
</table>
<table>
<thead>
<tr>
<th>struct</th><th>Field</th><th>Comment</th>
</tr>
</thead>
<tbody>
<tr>
<td>deal</td><td>int trump;</td><td>Suit encoding</td>
</tr>
<tr>
<td></td><td>int first;</td><td>The hand leading to the trick. Hand encoding.</td>
</tr>
<tr>
<td></td><td>int currentTrickSuit[3];</td><td>Up to 3 cards may already have been played to the trick. Suit encoding.</td>
</tr>
<tr>
<td></td><td>int currentTrickRank[3];</td><td>Up to 3 cards may already have been played to the trick. Value range 2-14. Set to 0 if no card has been played.</td>
</tr>
<tr>
<td></td><td>unsigned int remainCards[4][4];</td><td>1st index is Hand, 2nd index is Suit. remainCards is Holding encoding.</td>
</tr>
<tr>
<td colspan="3"></td>
</tr>
<tr>
<th>struct</th><th>Field</th><th>Comment</th>
</tr>
<tr>
<td>dealPBN</td><td>int trump;</td><td>Suit encoding</td>
</tr>
<tr>
<td></td><td>int first;</td><td>The hand leading to the trick. Hand encoding.</td>
</tr>
<tr>
<td></td><td>int currentTrickSuit[3];</td><td>Up to 3 cards may already have been played to the trick. Suit encoding. Set to 0 if no card has been played.</td>
</tr>
<tr>
<td></td><td>int currentTrickRank[3];</td><td>Up to 3 cards may already have been played to the trick. Value range 2-14. Set to 0 if no card has been played.</td>
</tr>
<tr>
<td></td><td>char remainCards[80];</td><td>Remaining cards. PBN encoding.</td>
</tr>
<tr>
<td colspan="3"></td>
</tr>
<tr>
<tr>
<th>struct</th><th>Field</th><th>Comment</th>
</tr>
<tr>
<td>ddTableDeal</td><td>unsigned int cards[4][4];</td><td>Encodes a deal. First index is hand. Hand encoding. Second index is suit. Suit encoding. Holding</td>
</tr>
<tr>
<td colspan="3"></td>
</tr>
<tr>
<tr>
<th>struct</th><th>Field</th><th>Comment</th>
</tr>
<tr>
<td>ddTableDeals</td><td>int noOfTables;</td><td>Number of DD table deals in structure, at most MAXNOOFTABLES</td>
</tr>
<tr>
<td></td><td>struct ddTableDeal deals[X];</td><td>X = MAXNOOFTABLES \* DDS_STRAINS</td>
</tr>
<tr>
<td colspan="3"></td>
</tr>
<tr>
<tr>
<th>struct</th><th>Field</th><th>Comment</th>
</tr>
<tr>
<td>ddTableDeals</td><td>int noOfTables;</td><td>Number of DD table deals in structure</td>
</tr>
<tr>
<td></td><td>struct ddTableDealPBN deals[MAXNOOFBOARDS/4];</td><td></td>
</tr>
<tr>
<td colspan="3"></td>
</tr>
<tr>
<tr>
<th>struct</th><th>Field</th><th>Comment</th>
</tr>
<tr>
<td>boards</td><td>int noOfBoards;</td><td>Number of boards</td>
</tr>
<tr>
<td></td><td>struct deal[MAXNOOFBOARDS];</td><td></td>
</tr>
<tr>
<td></td><td>int target[MAXNOOFBOARDS];</td><td>See SolveBoard</td>
</tr>
<tr>
<td></td><td>int solutions[MAXNOOFBOARDS];</td><td>See SolveBoard</td>
</tr>
<tr>
<td></td><td>int mode[MAXNOOFBOARDS];</td><td>See SolveBoard</td>
</tr>
<tr>
<td colspan="3"></td>
</tr>
<tr>
<tr>
<th>struct</th><th>Field</th><th>Comment</th>
</tr>
<tr>
<td>boardsPBN</td><td>int noOfBoards;</td><td>Number of boards</td>
</tr>
<tr>
<td></td><td>struct dealPBN[MAXNOOFBOARDS];</td><td></td>
</tr>
<tr>
<td></td><td>int target[MAXNOOFBOARDS];</td><td>See SolveBoard</td>
</tr>
<tr>
<td></td><td>int solutions[MAXNOOFBOARDS];</td><td>See SolveBoard</td>
</tr>
<tr>
<td></td><td>int mode[MAXNOOFBOARDS];</td><td>See SolveBoard</td>
</tr>
<tr>
<td colspan="3"></td>
</tr>
<tr>
<th>struct</th><th>Field</th><th>Comment</th>
</tr>
<tr>
<td>futureTricks</td><td>int nodes;</td><td>Number of nodes searched by the DD solver.</td>
</tr>
<tr>
<td></td><td>int cards;</td><td>Number of cards for which a result is returned.  May be all the cards, but equivalent ranks are omitted, so for a holding of KQ76 only the cards K and 7 would be returned, and the “equals” field below would be 2048 (Q) for the king and 54 (6) for the 7.</td>
</tr>
<tr>
<td></td><td>int suit[13];</td><td>Suit of the each returned card. Suit encoding.</td>
</tr>
<tr>
<td></td><td>int rank[13];</td><td>Rank of the returned card. Value range 2-14.</td>
</tr>
<tr>
<td></td><td>int equals[13];</td><td>Lower-ranked equals.  Holding encoding.</td>
</tr>
<tr>
<td></td><td>int score[13];</td><td>-1: target not reached. Otherwise: Target of maximum number of tricks.</td>
</tr>
<tr>
<td colspan="3"></td>
</tr>
<tr>
<tr>
<th>struct</th><th>Field</th><th>Comment</th>
</tr>
<tr>
<td>solvedBoards</td><td>int noOfBoards;</td><td></td>
</tr>
<tr>
<td></td><td>struct futureTricks solvedBoard [MAXNOOFBOARDS];</td><td></td>
</tr>
<tr>
<td colspan="3"></td>
</tr>
<tr>
<tr>
<th>Struct</th><th>Field</th><th>Comment</th>
</tr>
<tr>
<td>ddTableResults</td><td>int resTable[5][4];</td><td>Encodes the solution of a deal for combinations of denomination and declarer.  First index is denomination. Suit encoding.  Second index is declarer.  Hand encoding.  Each entry is a number of tricks.</td>
</tr>
<tr>
<td colspan="3"></td>
</tr>
<tr>
<tr>
<th>struct</th><th>Field</th><th>Comment</th>
</tr>
<tr>
<td>ddTablesRes</td><td>int noOfBoards;</td><td>Number of DD table deals in structure, at most MAXNOOFTABLES</td>
</tr>
<tr>
<td></td><td>struct ddTableResults results[X];</td><td>X = MAXNOOFTABLES \* DDS_STRAINS</td>
</tr>
<tr>
<td colspan="3"></td>
</tr>
<tr>
<tr>
<th>Struct</th><th>Field</th><th>Comment</th>
</tr>
<tr>
<td>parResults</td><td>char parScore[2][16];</td><td>First index is NS/EW. Side encoding.</td>
</tr>
<tr>
<td></td><td>char parContractsString[2][128];</td><td>First index is NS/EW.  Side encoding.</td>
</tr>
<tr>
<td colspan="3"></td>
</tr>
<tr>
<tr>
<th>Struct</th><th>Field</th><th>Comment</th>
</tr>
<tr>
<td>allParResults</td><td>struct parResults[MAXNOOFTABLES];</td><td>There are up to 20 declarer/strain combinations per DD table</td>
</tr>
<tr>
<td colspan="3"></td>
</tr>
<tr>
<tr>
<th>Struct</th><th>Field</th><th>Comment</th>
</tr>
<tr>
<td>parResultsDealer</td><td>int number;</td><td></td>
</tr>
<tr>
<td></td><td>int score;</td><td></td>
</tr>
<tr>
<td></td><td>char contracts[10][10];</td><td></td>
</tr>
<tr>
<td colspan="3"></td>
</tr>
<tr>
<tr>
<th>Struct</th><th>Field</th><th>Comment</th>
</tr>
<tr>
<td>parResultsMaster</td><td>int score;</td><td></td>
</tr>
<tr>
<td></td><td>int number;</td><td></td>
</tr>
<tr>
<td></td><td>struct contractType contracts[10];</td><td></td>
</tr>
<tr>
<td colspan="3"></td>
</tr>
<tr>
<tr>
<th>Struct</th><th>Field</th><th>Comment</th>
</tr>
<tr>
<td>contractType</td><td>int underTricks;</td><td></td>
</tr>
<tr>
<td></td><td>int overTricks;</td><td></td>
</tr>
<tr>
<td></td><td>int level;</td><td></td>
</tr>
<tr>
<td></td><td>int denom;</td><td></td>
</tr>
<tr>
<td></td><td>int seats;</td><td></td>
</tr>
<tr>
<td colspan="3"></td>
</tr>
<tr>
<tr>
<th>Struct</th><th>Field</th><th>Comment</th>
</tr>
<tr>
<td>parTextResults</td><td>char parText[2][128];</td><td></td>
</tr>
<tr>
<td></td><td>int equal;</td><td></td>
</tr>
<tr>
<td colspan="3"></td>
</tr>
<tr>
<tr>
<th>Struct</th><th>Field</th><th>Comment</th>
</tr>
<tr>
<td>playTraceBin</td><td>int number;</td><td>Number of cards in the play trace, starting from the beginning of the hand.</td>
</tr>
<tr>
<td></td><td>int suit[52];</td><td>Suit encoding.</td>
</tr>
<tr>
<td></td><td>int rank[52];</td><td>Encoding 2 .. 14 (not Card encoding).</td>
</tr>
<tr>
<td colspan="3"></td>
</tr>
<tr>
<tr>
<th>Struct</th><th>Field</th><th>Comment</th>
</tr>
<tr>
<td>playTracePBN</td><td>int number;</td><td>Number of cards in the play trace, starting from the beginning of the hand.</td>
</tr>
<tr>
<td></td><td>int cards[106];</td><td>String of cards with no space in between, also not between tricks.  Each card consists of a suit (C/D/H/S) and then a rank (2 .. A).  The string must be null-terminated.</td>
</tr>
<tr>
<td colspan="3"></td>
</tr>
<tr>
<tr>
<th>Struct</th><th>Field</th><th>Comment</th>
</tr>
<tr>
<td>playTracesBin</td><td>int noOfBoards;</td><td></td>
</tr>
<tr>
<td></td><td>struct playTraceBin plays[MAXNOOFBOARDS / 10];</td><td></td>
</tr>
<tr>
<td colspan="3"></td>
</tr>
<tr>
<tr>
<th>Struct</th><th>Field</th><th>Comment</th>
</tr>
<tr>
<td>playTracesPBN</td><td>int noOfBoards;</td><td></td>
</tr>
<tr>
<td></td><td>struct playTracePBN plays[MAXNOOFBOARDS / 10];</td><td></td>
</tr>
<tr>
<td colspan="3"></td>
</tr>
<tr>
<tr>
<th>Struct</th><th>Field</th><th>Comment</th>
</tr>
<tr>
<td>solvedPlay</td><td>int number;</td><td></td>
</tr>
<tr>
<td></td><td>int tricks[53];</td><td>Starting position and up to 52 cards</td>
</tr>
<tr>
<td colspan="3"></td>
</tr>
<tr>
<tr>
<th>Struct</th><th>Field</th><th>Comment</th>
</tr>
<tr>
<td>solvedPlays</td><td>int noOfBoards;</td><td></td>
</tr>
<tr>
<td></td><td>struct solvedPlay solved[MAXNOOFBOARDS];</td><td></td>
</tr>
</tbody>
</table>

## Functions

<table>
<thead>
<tr>
<th>SolveBoard</th><th>SolveoardPBN</th>
</tr>
</thead>
<tbody>
<tr>
<td>struct deal dl,</td><td>struct dealPBN dl,</td>
</tr>
<tr>
<td>int target,</td><td>int target,</td>
</tr>
<tr>
<td>int solutions,</td><td>int solutions,</td>
</tr>
<tr>
<td>int mode,</td><td>int mode,</td>
</tr>
<tr>
<td>struct futureTricks \*futp,</td><td>struct futureTricks \*futp,</td>
</tr>
<tr>
<td>int threadIndex</td><td>int threadIndex</td>
</tr>
</tbody>
</table>

SolveBoardPBN is just like SolveBoard, except for the input format. Historically it was one of the first functions, and it exposes the thread index directly to the user. Later functions generally don’t do that, and they also hide the implementation details such as transposition tables, see below.

SolveBoard solves a single deal “<code>dl</code>” and returns the result in “<code>\*futp</code>” which must be declared before calling SolveBoard.

If you have multiple hands to solve, it is always better to group them together into a single function call than to use SolveBoard.

SolveBoard is thread-safe, so several threads can call SolveBoard in parallel. Thus the user of DDS can create threads and call SolveBoard in parallel over them. The maximum number of threads is fixed in the DLL at compile time and is currently 16.  So “<code>threadIndex</code>” must be between 0 and 15 inclusive; see also the function SetMaxThreads.  Together with the PlayAnalyse functions, this is the only function that exposes the thread number to the user.

There is a “<code>transposition table</code>” memory associated with each thread.  Each node in the table is effectively a position after certain cards have been played and other certain cards remain.  The table is not deleted automatically after each call to SolveBoard, so it can be reused from call to call.  However, it only really makes sense to reuse the table when the hand is very similar in the two calls.  The function will still run if this is not the case, but it won’t be as efficient.  The reuse of the transposition table can be controlled by the “<code>mode</code>” parameter, but normally this is not needed and should not be done.

The three parameters “<code>target</code>”, “<code>solutions</code>” and “<code>mode</code>” together control the function.  Generally speaking, the target is the number of tricks to be won (at least) by the side to play; solutions controls how many solutions should be returned; and mode controls the search behavior.  See next page for definitions.

For equivalent cards, only the highest is returned, and lower equivalent cards are encoded in the <code>futureTricks</code> structure (see “<code>equals</code>”).

<table>
<thead>
<tr>
<th>target</th><th>solutions</th><th>comment</th>
</tr>
</thead>
<tbody>
<tr>
<td>-1</td><td>1</td><td>Find the maximum number of tricks for the side to
play.<br />Return only one of the optimum cards and its score.</td>
</tr>
<tr>
<td>-1</td><td>2</td><td>Find the maximum number of tricks for the side to
play.<td />Return all optimum cards and their scores.</td>
</tr>
<tr>
<td>0</td><td>1</td><td>Return only one of the cards legal to play, with
score set to 0.</td>
</tr>
<tr>
<td>0</td><td>2</td><td>Return all cards that legal to play, with score set to
0.</td>
</tr>
<tr>
<td>1 .. 13</td><td>1</td><td>If score is -1: Target cannot be reached. <br />
If score is 0: In fact no tricks at all can be won.<br />
If score is > 0: score will always equal target, even if more tricks can be won.<br />
One of the cards achieving the target is returned.</td>
</tr>
<tr>
<td>1 .. 13</td><td>2</td><td>Return all cards meeting (at least) the target.<br />
If the target cannot be achieved, only one card is returned with the score set as above.</td>
</tr>
<tr>
<td>any</td><td>3</td><td>Return all cards that can be legally played, with their scores in descending order.</td>
</tr>
<tr>
<td colspan="3">&nbsp;</td>
</tr>
<th>mode</th><th>Reuse TT?</th><th>comment</th>
</tr>
<tr>
<td>0</td><td rowspan="2">Automatic if same trump suit and the same or nearly the same cards distribution, deal.first can be different.</td><td>Do not search to find the core if the hand to play has only one card, including its equivalents, to play. Score is set to –2 for this card, indicating that there are no alternative cards. If there are multiple choices for cards to play, search is done to find the score. This mode is very fast but you don’t always search to find the score.</td>
</tr>
<tr>
<td>1</td><td rowspan="2">Always search to find the score. Even when the hand to play has only one card, with possible equivalents, to play.</td>
</tr>
<tr>
<td>2</td><td>Always</td>
</tr>
</tbody>
</table>

Note: mode no longer always has this effect internally in DDS. We think mode is no longer useful,
and we may use it for something else in the future. If you think you need it, let us know!

“Reuse” means “reuse the transposition table from the previous run with the same thread number”.
For mode = 2 it is the responsibility of the programmer using the DLL to ensure that reusing the table is safe in the actual situation. Example: Deal is the same, except for `deal.first`. The Trump suit is the same.

1<sup>st</sup> call, East leads: `SolveBoard(deal, -1, 1, 1, &fut, 0), deal.first=1`

2<sup>nd</sup> call, South leads: `SolveBoard(deal, -1, 1, 2, &fut, 0), deal.first=2`

3<sup>rd</sup> call, West leads: `SolveBoard(deal, -1, 1, 2, &fut, 0), deal.first=3`

4<sup>th</sup> call, North leads: `SolveBoard(deal, -1, 1, 2, &fut, 0), deal.first=0`

<table>
<thead>
<tr>
<th>CalcDDtable</th><th>CalcDDtablePBN</th>
</tr>
</thead>
<tbody>
<tr>
<td>struct ddTableDeal tableDeal</td><td>struct ddTableDealPBN tableDealPBN</td>
</tr>
<tr>
<td>struct ddTableResults \* tablep</td><td>struct ddTableResults \* tablep</td>
</tr>
</tbody>
</table>

CalcDDtablePBN is just like CalcDDtable, except for the input format. 
CalcDDtable solves a single deal “ tableDeal ” and returns the double-dummy values for the initial 52 cards for all the 20 combinations of denomination and declarer in “ \*tablep” , which must be declared before calling CalcDDtable.

<table>
<thead>
<tr>
<th>CalcAllTables</th><th>CalcAllTablesPBN</th>
</tr>
</thead>
<tbody>
<tr>
<td>struct ddTableDeals \*dealsp</td><td>struct ddTableDealsPBN \*dealsp</td>
</tr>
<tr>
<td>int mode</td><td>int mode</td>
</tr>
<tr>
<td>int trumpFilter[5]</td><td>int trumpFilter[5]</td>
</tr>
<tr>
<td>struct ddTablesRes \*resp</td><td>struct ddTablesRes \*resp</td>
</tr>
<tr>
<td>struct allParResults \*presp</td><td>struct allParResults \*presp</td>
</tr>
</tbody>
</table>

CalcAllTablesPBN is just like CalcAllTables, except for the input format.

CallAllTables calculates the double dummy values of the denomination/declarer hand combinations in “\*dealsp” for a number of DD tables in parallel. This increases the speed compared to calculating these values using a CalcDDtable call for each DD table. The results are returned in “\*resp” which must be defined before CalcAllTables is called.

The “mode” parameter contains the vulnerability (Vulnerable encoding; not to be confused with the SolveBoard mode) for use in the par calculation. It is set to -1 if no par calculation is to be performed.

There are 5 possible denominations or strains (the four trump suits and no trump). The parameter “trumpFilter” describes which, if any, of the 5 possibilities that will be excluded from the calculations. They are defined in Suit encoding order, so setting trumpFilter to {FALSE, FALSE, TRUE, TRUE, TRUE} means that values will only be calculated for the trump suits spades and hearts.

The maximum number of DD tables in a CallAllTables call depends on the number of strains required, see the following table:

<table>
<thead>
<tr>
<td>Number of strains</td><td>Maximum number of DD tables</td>
</tr>
</thead>
<tbody>
<tr>
<td>5</td><td>32</td>
</tr>
<tr>
<td>4</td><td>40</td>
</tr>
<tr>
<td>3</td><td>53</td>
</tr>
<tr>
<td>2</td><td>80</td>
</tr>
<tr>
<td>1</td><td>160</td>
</tr>
</tbody>
</table>

<table>
<thead>
<tr>
<th>SolveAllBoards</th><th>SolverAllChunksBin</th><th>SolveAllChunksPBN</th>
</tr>
</thead>
<tbody>
<tr>
<td>struct boards \*bop</td><td>struct boards \*bop</td><td>struct boardsPBN \*bop</td>
</tr>
<tr>
<td>struct solvedBoards \* solvedp</td><td>struct solvedBoards \*solvedp</td><td>struct solvedBoards \*solvedp</td>
</tr>
<tr>
<td>int chunkSize</td><td>int chunkSize</td>
</tr>
</tbody>
</table>

`SolveAllChunks` is an alias for SolveAllChunksPBN; don’t use it.

`SolveAllBoards` used to be an alias for SolveAllChunksPBN with a chunkSize of 1; however this has been changed in v2.8, and we now recommend only to use SolveAllBoards and not the chunk functions any more; explanation follows.

The SolveAll\* functions invoke SolveBoard several times in parallel in multiple threads, rather than sequentially in a single thread. This increases execution speed. Up to 200 boards are permitted per call.

It is important to understand the parallelism and the concept of a chunk.

If the chunk size is 1, then each of the threads starts out with a single board. If there are four threads, then boards 0, 1, 2 and 3 are initially solved. If thread 2 is finished first, it gets the next available board, in this case board 4. Perhaps this is a particularly easy board, so thread 2 also finishes this board before any other thread completes. Thread 2 then also gets board 5, and so on. This continues until all boards have been solved. In the end, three of the threads will be waiting for the last thread to finish, which causes a bit of inefficiency.

The transposition table in a given thread (see SolveBoard) is generally not reused between board 2, 4 and 5 in thread 2. This only happens if SolveBoard itself determines that the boards are suspiciously similar. If the chunk size is 2, then initially thread 0 gets boards 0 and 1, thread 1 gets boards 2 and 3, thread 2 gets boards 4 and 5, and thread 3 gets boards 6 and 7. When a thread is finished, it gets two new boards in one go, for instance boards 8 and 9. The transposition table in a given thread is reused within a chunk.

No matter what the chunk size is, the boards are solved in parallel. If the user knows that boards are grouped in chunks of 2 or 10, it is possible to force the DD solver to use this knowledge. However, this is rather limiting on the user, as the alignment must remain perfect throughout the batch.

SolveAllBoards now detects repetitions automatically within a batch, whether or not the hands are evenly arranged and whether or not the duplicates are next to each other. This is more flexible and transparent to the user, and the overhead is negligible. Therefore, use SolveAllBoards!

<table>
<thead>
<tr>
<th>Par</th><th>DealerPar</th>
</tr>
</thead>
<tbody>
<tr>
<td>struct ddTableResults \*tablep</td><td>struct ddTableResults \*tablep</td>
</tr>
<tr>
<td>struct parResults \*presp</td><td>struct parResultsDealer \*presp</td>
</tr>
<tr>
<td>int vulnerable</td><td>int dealer</td>
</tr>
<tr>
<td>&nbsp;</td><td>int vulnerable</td>
</tr>
<tr>
<td colspan="2">&nbsp;</td>
</tr>
<tr>
<th>Sidespar</th><th>&nbsp;</th>
</tr>
<tr>
<td>struct ddTableResults \*tablep</td><td>&nbsp;</td>
</tr>
<tr>
<td>struct parResultsDealer \*sidesRes[2]</td><td>&nbsp;</td>
</tr>
<tr>
<td>int vulnerable</td><td>&nbsp;</td>
</tr>
<tr>
<td colspan="2">&nbsp;</td>
</tr>
<tr>
<th>DealerParBin</th><th>SidesParBin</th>
</tr>
<tr>
<td>struct ddTableResults \*tablep</td><td>struct ddTableResults \*tablep</td>
</tr>
<tr>
<td>struct parResultsMaster \* presp</td><td>struct parResultsMaster \* presp</td>
</tr>
<tr>
<td>int vulnerable</td><td>int dealer</td>
</tr>
<tr>
<td>&nbsp;</td><td>int vulnerable</td>
</tr>
<tr>
<td colspan="2">&nbsp;</td>
</tr>
<tr>
<th>ConvertToDealerTextForamat</th><th>ConvertToSidesTextFormat</th>
</tr>
<tr>
<td>struct parResultsMaster \*pres</td><td>struct parResultsMaster \*pres</td>
</tr>
<tr>
<td>char \*resp</td><td>struct parTextResults \*resp</td>
</tr>
</tbody>
</table>

The functions Par, DealerPar, SidesPar, DealerParBin and SidesParBin calculate the par score and par contracts of a given double-dummy solution matrix `*tablep` which would often be the solution of a call to CalcDDtable. Since the input is a table, there is no PBN and non-PBN version of these functions.

Before the functions can be called, a structure of the type “parResults” , `parResultsDealer` or `parResultsMaster` must already have been defined. 

The `vulnerable` parameter is given using Vulnerable encoding.

The Par() function uses knowledge of the vulnerability, but not of the dealer. It attempts to return results for both declaring sides. These results can be different in some rare cases, for instance when both sides can make 1NT due to the opening lead.

The DealerPar() function also uses knowledge of the `dealer` using Hand encoding. The  rgument is that in all practical cases, the dealer is known when the vulnerability is known. Therefore all results returned will be for the same side.

The SidesPar() function is similar to the Par() function, the only difference is that the par results are given in the same format as for DealerPar().

In Par() and SidesPar() there may be more than one par score; in DealerPar() that is not the case. Par() returns the scores as a text string, for instance “NS -460”, while DealerPar() and SidesPar() use an integer, -460.

There may be several par contracts, for instance 3NT just making and 5C just making. Each par contract is returned as a text string. The formats are a bit different betweeen the two output format alternatives.

Par() returns the par contracts separated by commas. Possible different trick levels of par score contracts are enumerated in the contract description, e.g the possible trick levels 3, 4 and 5 in no trump are given as 345N. Examples:

* “NS:NS 23S,NS 23H”. North and South as declarer make 2 or 3 spades and hearts contracts, 2 spades and 2 hearts with an overtrick. This is from the NS view, shown by “NS:” meaning that NS made the first bid. Note that this information is actually not enough, as it may be that N and S can make a given contract and that either E or W can bid this same contract (for instance 1NT) before N but not before S. So in the rare cases where the NS and EW sides are not the same, the results will take some manual inspection.
* “NS:NS 23S,N 23H”: Only North makes 3 hearts.
* “EW:NS 23S,N 23H”: This time the result is the same when EW open the bidding.

DealerPar() and SidesPar() give each par contract as a separate text string:

* “4S*-EW-1” means that E and W can both sacrifice in four spades doubled, going down one trick.
* “3N-EW” means that E and W can both make exactly 3NT.
* “4N-W+1” means that only West can make 4NT +1. In the last example, 5NT just making can also be considered a par contract, but North-South don’t have a profitable sacrifice against 4NT, so the par contract is shown in this way. If North-South did indeed have a profitable sacrifice, perhaps 5C\*_NS-2, then par contract would have been shown as “5N-W”. Par() would show “4N-W+1” as “W 45N”.
* SidesPar() give the par contract text strings as described above for each side.

DealerParBin and SidesParBin are similar to DealerPar and SidesPar, respectively, except that both functions give the output results in binary using the `parResultsMaster` structure. This simplifies the writing of a conversion program to get an own result output format. Examples of such programs are ConvertToDealerTextFormat and  ConvertToSidesTextFormat.

After DealerParBin or SidesParBin is called, the results in parResultsMaster are used when calling ConvertToDealerTextFormat resp. ConvertToSidesTextFormat.
Output example from ConvertToDealerTextFormat:

“Par 110: NS 2S NS 2H”

Output examples from ConvertToSidesTextFormat:

“NS Par 130: NS 2D+2 NS 2C+2” when it does not matter who starts the bidding.
”NS Par -120: W 2NT<br />
EW Par 120: W 1NT+1” when it matters who starts the bidding.

<table>
<thead>
<tr>
<th>AnalysePlayBin</th><th>AnalysePlayPBN</th>
</tr>
</thead>
<tbody>
<tr>
<td>struct deal dl</td><td>struct dealPBN dlPBN</td>
</tr>
<tr>
<td>struct playTraceBin play</td><td>struct playTracePBN playPBN</td>
</tr>
<tr>
<td>struct solvedPlay \*solvedp</td><td>struct solvedPlay \*solvedp</td>
</tr>
<tr>
<td>int thrId</td><td>int thrId</td>
</tr>
</tbody>
</table>

AnalysePlayPBN is just like AnalysePlayBin, except for the input format.

The function returns a list of double-dummy values after each specific played card in a hand. Since the function uses SolveBoard, the same comments apply concerning the thread number `thrId` and the transposition tables.

As an example, let us say the DD result in a given contract is 9 tricks for declarer. The play consists of the first trick, two cards from the second trick, and then declarer claims. The lead and declarer’s play to the second trick (he wins the first trick) are sub-optimal. Then the trace would look like this, assuming each sub-optimal costs 1 trick:

9 10 10 10 10 9 9

The number of tricks are always seen from declarer’s viewpoint (he is the one to the right of the opening leader). There is one more result in the trace than there are cards played, because there is a DD value before any card is played, and one DD value after each card played.

<table>
<thead>
<tr>
<th>AnalyseAllPlaysBin</th><th>AnalyseAllPlaysPBN</th>
</tr>
</thead>
<thead>
<tr>
<td>struct boards \*bop</td><td>struct boardsPBN \*bopPBN</td>
</tr>
<tr>
<td>struct playTracesBin \*plp</td><td>struct playTracesPBN \*plpPBN</td>
</tr>
<tr>
<td>struct solvedPlays \*solvedp</td><td>struct solvedPlays \*solvedp</td>
</tr>
<tr>
<td>int chunkSize</td><td>int chunkSize</td>
</tr>
</thead>
</table>

AnalyseAllPlaysPBN is just like AnalyseAllPlaysBin, except for the input format.

The AnalyseAllPlays\* functions invoke SolveBoard several times in parallel in multiple threads, rather than sequentially in a single thread. This increases execution speed. Up to 20 boards are permitted per call.

Concerning chunkSize, exactly the 21 same remarks apply as with SolveAllChunksBin.

<table>
<thead>
<tr>
<th>SetMaxThreads</th><th>FreeMemory</th>
</tr>
</thead>
<tbody>
<tr>
<td>int userThreads</td><td>void</td>
</tr>
</tbody>
</table>

SetMaxThreads returns the actual number of threads.

DDS has a preferred memory size per thread, currently about 95 MB, and a maximum memory size per thread, currently about 160 MB. It will also not use more than 70% of the available memory. It will not create more threads than there are processor cores, as this will only require more memory and will not improve performance. Within these constraints, DDS auto-configures the
number of threads.

DDS first detects the number of cores and the available memory. If this doesn't work for some reason, it defaults to 1 thread which is allowed to use the maximum memory size per thread.

DDS then checks whether a number of threads equal to the number of cores will fit within the available memory when each thread may use the maximum memory per thread. If there is not enough memory for this, DDS scales back its ambition. If there is enough memory for the preferred memory size, then DDS still creates a number of threads equal to the number of cores. If there is not even enough memory for this, DDS scales back the number of threads to fit within the memory.

The user can suggest to DDS a number of threads by calling SetMaxThreads. DDS will never create more threads than requested, but it may create fewer if there is not enough memory, calculated as above. Calling SetMaxThreads is optional, not mandatory. DDS will always select a suitable number of threads on its own.

It may be possible, especially on non-Windows systems, to call SetMaxThreads() actively, even though the user does not want to influence the default values. In this case, use a 0 argument.

SetMaxThreads can be called multiple times even within the same session. So it is theoretically possible to change the number of threads dynamically. 

It is possible to ask DDS to give up its dynamically allocated memory by calling FreeMemory. This could be useful for instance if there is a long pause where DDS is not used within a session. DDS will free its memory when the DLL detaches from the user program, so there is no need for the user to call this function before detaching.

## Return codes

<table>
<thead>
<tr>
<th>Value</th><th>Code</th><th>Comment</th>
</tr>
</thead>
<tbody>
<tr>
<td>1</td><td>RETURN_NO_FAULT</td><td>&nbsp;</td>
</tr>
<tr>
<td>-1</td><td>RETURN_UNKNOWN_FAULT</td><td>Currently happens when fopen() returns an error or when AnalyseAllPlaysBin() gets a
different number of boards in its first two arguments.</td>
</tr>
<tr>
<td>-2</td><td>RETURN_ZERO_CARDS</td><td>SolveBoard(), self-explanatory.</td>
</tr>
<tr>
<td>-3</td><td>RETURN_TARGET_TOO_HIGH</td><td>SolveBoard(), target is higher than the number of tricks remaining.</td>
</tr>
<tr>
<td>-4</td><td>RETURN_DUPLICATE_CARDS</td><td>SolveBoard(), self-explanatory.</td>
</tr>
<tr>
<td>-5</td><td>RETURN_TARGET_WRONG_LO</td><td>SolveBoard(), target is less than -1.</td>
</tr>
<tr>
<td>-7</td><td>RETURN_TARGET_WRONG_HI</td><td>SolveBoard(), target is higher than 13.</td>
</tr>
<tr>
<td>-8</td><td>RETURN_SOLNS_WRONG_LO</td><td>SolveBoard(), solutions is less than 1.</td>
</tr>
<tr>
<td>-10</td><td>RETURN_SOLNS_WRONG_HI</td><td>SolveBoard(), solutions is higher than 3.</td>
</tr>
<tr>
<td>-11</td><td>RETURN_TOO_MANY_CARDS</td><td>SolveBoard(), self-explanatory.</td>
</tr>
<tr>
<td>-12</td><td>RETURN_SUIT_OR_RANK</td><td>SolveBoard(), either currentTrickSuit or currentTrickRank have wrong data.</td>
</tr>
<tr>
<td>-13</td><td>RETURN_PLAYED_CARD</td><td>SolveBoard(), card already played is also a card still remaining to play.</td>
</tr>
<tr>
<td>-14</td><td>RETURN_CARD_COUNT</td><td>SolveBoard(), wrong number of remaining cards for a hand.</td>
</tr>
<tr>
<td>-15</td><td>RETURN_THREAD_INDEX</td><td>SolveBoard(), thread number is less than 0 or higher than the maximum permitted.</td>
</tr>
<tr>
<td>-16</td><td>RETURN_MODE_WRONG_LO</td><td>SolveBoard(), mode is less than 0</td>
</tr>
<tr>
<td>-17</td><td>RETURN_MODE_WRONG_HI</td><td>SolveBoard(), mode is greater than 2</td>
</tr>
<tr>
<td>-18</td><td>RETURN_TRUMP_WRONG</td><td>SolveBoard(), trump is not one or 0, 1, 2, 3, 4</td>
</tr>
<tr>
<td>-19</td><td>RETURN_FIRST_WRONG</td><td>SolveBoard(), first is not one or 0, 1, 2</td>
<tr>
<td>-98</td><td>RETURN_PLAY_FAULT</td><td>AnalysePlay\*() family of functions. (a) Less than 0 or more than 52 cards supplied. (b)
Invalid suit or rank supplied. (c) A played card is not held by the right player.</td>
</tr>
<tr>
<td>-99</td><td>RETURN_PBN_FAULT</td><td>Returned from a number of places if a PBN string is faulty.</td>
</tr>
<tr><td>-101</td><td>RETURN_TOO_MANY_THREADS</td><td>Currently never returned.</td>
</tr>
<tr>
<td>-102</td><td>RETURN_THREAD_CREATE</td><td>Returned from multi-threading functions.</td>
</tr>
<tr>
<td>-103</td><td>RETURN_THREAD_WAIT</td><td>Returned from multi-threading functions when something went wrong while waiting for all threads to complete.</td>
</tr>
<tr>
<td>-201</td><td>RETURN_NO_SUIT</td><td>CalcAllTables\*(), returned when the denomination filter vector has no entries.</td>
</tr>
<tr>
<td>-202</td><td>RETURN_TOO_MANY_TABLES</td><td>CalcAllTables\*(), returned when too many tables are requested.</td>
</tr>
<tr>
<td>-301</td><td>RETURN_CHUNK_SIZE</td><td>SolveAllChunks\*(), returned when the chunk size is < 1.</td>
</tr>
</tbody>
</table>

<table>
<thead>
<tr>
<th colspan="3">Revision History</th>
</tr>
</thead>
<tbody>
<tr>
<td>Rev A</td><td>2006-02-25</td><td>First issue</td>
</tr>
<tr>
<td>Rev B</td><td>2006-03-20</td><td>Updated issue</td>
</tr>
<tr>
<td>Rev C</td><td>2006-03-28</td><td>Updated issue. Addition of the SolveBoard parameter ”mode”</td>
</tr>
<tr>
<td>Rev D</td><td>2006-04-05</td><td>Updated issue. Usage of target=0 to list all cards that are legal to play</td>
</tr>
<tr>
<td>Rev E</td><td>2006-05-29</td><td>Updated issue. New error code –10 for number of cards > 52</td>
</tr>
<tr>
<td>Rev F</td><td>2006-08-09</td><td>Updated issue. New mode parameter value = 2. New error code –11 for calling SolveBoard with mode = 2 and forbidden values of other parameters</td>
</tr>
<tr>
<td>Rev F1</td><td>2006-08-14</td><td>Clarifications on conditions for returning scores for the different combinations of the values for target and solutions</td>
</tr>
<tr>
<td>Rev F2</td><td>2006-08-26</td><td>New error code –12 for wrongly set values of deal.currentTrickSuit and deal.currentTrickRank</td>
</tr>
<tr>
<td>Rev G</td><td>2007-01-04</td><td>New DDS release 1.1, otherwise no change compared to isse F2</td>
</tr>
<tr>
<td>Rev H</td><td>2007-04-23</td><td>DDS release 1.4, changes for parameter mode=2.</td>
</tr>
<tr>
<td>Rev I</td><td>2010-04-10</td><td>DDS release 2.0, multi-thread support</td>
</tr>
<tr>
<td>Rev J</td><td>2010-05-29</td><td>DDS release 2.1, OpenMP support, reuse of previous DD transposition table results of similar deals</td>
</tr>
<tr>
<td>Rev K</td><td>2010-10-27</td><td>Correction of fault in the description: 2nd index in resTable of the structure ddTableResults is declarer hand</td>
</tr>
<tr>
<td>Rev L</td><td>2011-10-14</td><td>Added SolveBoardPBN and CalcDDtablePBN</td>
</tr>
<tr>
<td>Rev M</td><td>2012-07-06</td><td>Added SolveAllBoards.
Rev N, 2012-07-16 Max number of threads is 8</td>
</tr>
<tr>
<td>Rev O</td><td>2012-10-21</td><td>Max number of threads is configured at initial start-up, but never exceeds 16</td>
</tr>
<tr>
<td>Rev P</td><td>2013-03-16</td><td>Added functions CalcPar and CalcParPBN</td>
</tr>
<tr>
<td>Rev Q</td><td>2014-01-09</td><td>Added functions CalcAllTables/CalcAllTablesPBN</td>
</tr>
<tr>
<td>Rev R</td><td>2014-01-13</td><td>Updated functions CalcAllTables/CalcAllTablesPBN</td>
</tr>
<tr>
<td>Rev S</td><td>2014-01-13</td><td>Updated functions CalcAllTables/CalcAllTablesPBN</td>
</tr>
<tr>
<td>Rev T</td><td>2014-03-01</td><td>Added function SolveAllChunks</td>
</tr>
<tr>
<td>Rev U</td><td>2014-09-15</td><td>Added functions DealerPar, SidesPar, AnalysePlayBin, AnalysePlayPBN, AnalyseAllPlaysBin,
AnalyseAllPlaysPBN</td>
</tr>
<tr>
<td>Rev V</td><td>2014-10-14</td><td>Added functions SetMaxThreads, FreeMemory, DealerParBin, SidesParBin,
ConvertToDealerTextFormat, ConvertToSidesTextFormat</td>
</tr>
<tr>
<td>Rev X</td><td>2014-11-16</td><td>Extended maximum number of tables when calling CalcAllTables.</td>
</tbody>
</table>
