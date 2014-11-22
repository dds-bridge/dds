/* 
   DDS 2.7.0   A bridge double dummy solver.
   Copyright (C) 2006-2014 by Bo Haglund   
   Cleanups and porting to Linux and MacOSX (C) 2006 by Alex Martelli.
   The code for calculation of par score / contracts is based upon the
   perl code written by Matthew Kidd for ACBLmerge. He has kindly given
   permission to include a C++ adaptation in DDS.
   						
   The PlayAnalyser analyses the played cards of the deal and presents 
   their double dummy values. The par calculation function DealerPar 
   provides an alternative way of calculating and presenting par 
   results.  Both these functions have been written by Soren Hein.
   He has also made numerous contributions to the code, especially in 
   the initialization part.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
   http://www.apache.org/licenses/LICENSE-2.0
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or 
   implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/


#include "dll.h"
#include "dds.h"
#include "Init.h"


#ifdef _MANAGED
#pragma managed(push, off)
#endif


#if defined(_WIN32) || defined(USES_DLLMAIN)

extern "C" BOOL APIENTRY DllMain(HMODULE hModule,
  DWORD ul_reason_for_call, LPVOID lpReserved) 
{

  if (ul_reason_for_call==DLL_PROCESS_ATTACH) 
  {
    SetMaxThreads(0);
  }
  else if (ul_reason_for_call==DLL_PROCESS_DETACH) 
  {
    CloseDebugFiles();
    FreeMemory();
#ifdef DDS_MEMORY_LEAKS_WIN32
    _CrtDumpMemoryLeaks();
#endif
  }
  return 1;
}

#elif defined(USES_CONSTRUCTOR)

void __attribute__ ((constructor)) libInit(void) 
{
  SetMaxThreads(0);
}


void __attribute__ ((destructor)) libEnd(void) 
{
  CloseDebugFiles();
  FreeMemory();
}

#endif

#ifdef _MANAGED
#pragma managed(pop)
#endif

