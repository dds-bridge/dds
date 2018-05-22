/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include "../include/dll.h"
#include "Init.h"

#ifdef _MANAGED
  #pragma managed(push, off)
#endif


#if defined(_WIN32) || defined(USES_DLLMAIN)

extern "C" BOOL APIENTRY DllMain(
  HMODULE hModule,
  DWORD ul_reason_for_call,
  LPVOID lpReserved);

extern "C" BOOL APIENTRY DllMain(
  HMODULE hModule,
  DWORD ul_reason_for_call,
  LPVOID lpReserved)
{

  if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    SetMaxThreads(0);
  else if (ul_reason_for_call == DLL_PROCESS_DETACH)
  {
    CloseDebugFiles();
    FreeMemory();
#ifdef DDS_MEMORY_LEAKS_WIN32
    _CrtDumpMemoryLeaks();
#endif
  }

  UNUSED(hModule);
  UNUSED(lpReserved);

  return 1;
}

#elif (defined(__IPHONE_OS_VERSION_MAX_ALLOWED) || defined(__MAC_OS_X_VERSION_MAX_ALLOWED))

void DDSInitialize(), DDSFinalize();

void DDSInitialize(void) 
{
  SetMaxThreads(0);
}


void DDSFinalize(void) 
{
  CloseDebugFiles();
  FreeMemory();
}

#elif defined(USES_CONSTRUCTOR)

static void __attribute__ ((constructor)) libInit(void)
{
  SetMaxThreads(0);
}


static void __attribute__ ((destructor)) libEnd(void)
{
  CloseDebugFiles();
  FreeMemory();
}

#endif

#ifdef _MANAGED
  #pragma managed(pop)
#endif

