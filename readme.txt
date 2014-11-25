DDS 2.3.0,  Bo Haglund 2013-03-18

For Win32, DDS compiles with Visual C++ 2010 Express edition 
and the TDM-GCC/Mingw port of gcc.

When using Visual C++, the statement
#include "stdafx.h" at the beginning of dds.cpp must be uncommented.

When not using Visual C++, the compilation of DDS includes function CalcDDtable
implemented using the Win API for Windows and TDM-GCC/MingW OpenMP otherwise. 
   

Linking with an application using DDS
-------------------------------------
The InitStart() function of DDS should be called from inside the
application. Then SolveBoard in DDS can be called multiple times
without the overhead of InitStart() at each call.
For this purpose, the application code must have an include
statement for the dll.h file in DDS.


Maximum number of threads
------------------------- 
The maximum number of simultaneous threads depends on the PC physical memory size:
1 GB or less, max 2 threads.
2 GB, max 4 threads.
3 or 4 GB, max 16 threads.
 
For e.g. Windows, allocating memory for the maximum number of simultaneous threads can 
be done by reading out the physical memory size from Windows. This is done in the DDS.DLL.
Another alternative is to provide the physical memory size as a parameter (gb_ram) in the
InitStart call. This alternative needs to be used when the operating system does not support
memory autoconfiguration.


Setting the number of simultaneous threads when calling CalcDDtable.
--------------------------------------------------------------------
For Windows, this can be done either by reading out the number of processor cores from
Windows and using this for setting the number of threads, or by supplying the number of
threads (ncores) in InitStart. This latter alternative needs to be used when the operating 
system does not support reading out of the number of processors. 
 

Options at DDS compilation
--------------------------
Compiling options:

The SolveBoard and CalcDDtable are included in all DDS compilation options.
The "PBN" and the "PBN_PLUS" definitions are included in the header fill dll.h.
Defining "PBN" means that the functions SolveBoardPBN and CalcDDtablePBN are supported.
Defining "PBN_PLUS" as well means that also the SolveAllBoards function,
ParCalc and ParCalcPBN are supported.

The possible configurens thus are:
1)  "PBN" and "PBN_PLUS":  Support for SolveBoardPBN, CalcDDtablePBN, SolveAllBoards,
   ParCalc and ParCalcPBN.
2)  "PBN":  Support for SolveBoardPBN and CalcDDtablePBN.
3) None of these definitions:  No support for any of the above 3 functions.

Staying with the previous configuration might be needed when 2.3.0 is to replace an 
older 2.x.y version, and the application using DDS cannot handle a changed interface.




