DDS 2.7,  Bo Haglund 2014-10-18

For Win32, DDS compiles with Visual C++ 2010 Express editions or later editions 
and the TDM-GCC/Mingw port of gcc.

   

Linking with an application using DDS
-------------------------------------
The SetMaxThreads() function of DDS should be called from inside the
application. Then SolveBoard in DDS can be called multiple times
without the overhead of SetMaxThreads() at each call.
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
For Windows, this can be done either by reading out the number of processor cores 
and using this for setting the number of threads, or by supplying the number of
threads (ncores) in SetMaxThreads. This latter alternative needs to be used when the operating 
system does not support reading out of the number of processors. 
 






