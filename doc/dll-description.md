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
### Data sructure
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
<td></td><td>unsigned int remainCards[4][4];</td><td>1st index is Hand, 2nd index is Suit.  remainCards is Holding encoding.</td>
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
<td>ddTableDeals</td><td>int noOfTables;</td><td>Number of DD table deals in structure</td>
</tr>
<tr>
<td></td><td>struct ddTableDeal deals[MAXNOOFBOARDS/4];</td><td></td>
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
<td></td><td>int equals[13];</td><td>Lower-ranked equals.  Holding PBN encoding.</td>
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
<th>struct</th><th>Field</th><th>Comment</th>
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
<td>ddTablesRes</td><td>int noOfBoards;</td><td></td>
</tr>
<tr>
<td></td><td>struct ddTableResults results[MAXNOOFBOARDS/4];</td><td></td>
</tr>
<tr>
<td colspan="3"></td>
</tr>
<tr>
<tr>
<th>struct</th><th>Field</th><th>Comment</th>
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
<th>struct</th><th>Field</th><th>Comment</th>
</tr>
<tr>
<td>allParResults</td><td>struct parResults[MAXNOOFBOARDS/20];</td><td></td>
</tr>
<tr>
<td colspan="3"></td>
</tr>
<tr>
<tr>
<th>struct</th><th>Field</th><th>Comment</th>
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
<th>struct</th><th>Field</th><th>Comment</th>
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
<th>struct</th><th>Field</th><th>Comment</th>
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
<th>struct</th><th>Field</th><th>Comment</th>
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
<th>struct</th><th>Field</th><th>Comment</th>
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
<th>struct</th><th>Field</th><th>Comment</th>
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
<th>struct</th><th>Field</th><th>Comment</th>
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
<th>struct</th><th>Field</th><th>Comment</th>
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
<th>struct</th><th>Field</th><th>Comment</th>
</tr>
<tr>
<td>solvedPlay</td><td>int number;</td><td></td>
</tr>
<tr>
<td></td><td>int tricks[53];</td><td></td>
</tr>
<tr>
<td colspan="3"></td>
</tr>
<tr>
<tr>
<th>struct</th><th>Field</th><th>Comment</th>
</tr>
<tr>
<td>solvedPlays</td><td>int noOfBoards;</td><td></td>
</tr>
<tr>
<td></td><td>struct solvedPlay solved[MAXNOOFBOARDS / 10];</td><td></td>
</tr>
</tbody>
</table>

### Functions
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
<td>struct futureTricks *futp,</td><td>struct futureTricks *futp,</td>
</tr>
<tr>
<td>int threadIndex</td><td>int threadIndex</td>
</tr>
</tbody>
</table>
SolveBoardPBN is just like SolveBoard, except for the input format.

SolveBoard solves a single deal “<code>dl</code>” and returns the result in “<code>*futp</code>” which must be declared before calling SolveBoard.

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
“Reuse” means “reuse the transposition table from the previous run with the same thread number”.
For mode = 2 it is the responsibility of the programmer using the DLL to ensure that reusing the table is safe in the actual situation. Example: Deal is the same, except for deal.first. Trump suit is the same.

1 st call, East leads: `SolveBoard(deal, -1, 1, 1, &fut, 0), deal.first=1`
2 nd call, South leads: `SolveBoard(deal, -1, 1, 2, &fut, 0), deal.first=2`
3rd call, West leads: `SolveBoard(deal, -1, 1, 2, &fut, 0), deal.first=3`
4th call, North leads: `SolveBoard(deal, -1, 1, 2, &fut, 0), deal.first=0`

