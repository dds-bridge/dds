/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <api/dll.h>
#include "Init.h"

#ifdef _MANAGED
  #pragma managed(push, off)
#endif


#if defined(_WIN32) || defined(USES_DLLMAIN)

extern "C" BOOL APIENTRY DllMain(
  [[maybe_unused]] HMODULE hModule,
  DWORD ul_reason_for_call,
  [[maybe_unused]] LPVOID lpReserved);

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

  hModule;
  lpReserved;

  return 1;
}

#elif (defined(__IPHONE_OS_VERSION_MAX_ALLOWED) || defined(__MAC_OS_X_VERSION_MAX_ALLOWED))

/**
 * @brief Initialize the DDS library (Apple platforms).
 */
void DDSInitialize(), DDSFinalize();

/**
 * @brief Initialize the DDS library.
 */
void DDSInitialize(void) 
{
  SetMaxThreads(0);
}


/**
 * @brief Finalize and clean up the DDS library.
 */
void DDSFinalize(void) 
{
  CloseDebugFiles();
  FreeMemory();
}

#elif defined(USES_CONSTRUCTOR)

/**
 * @brief Library constructor for platforms supporting constructor attribute.
 *
 * This function is called when the library is loaded.
 */
static void __attribute__ ((constructor)) libInit(void)
{
  SetMaxThreads(0);
}


/**
 * @brief Library destructor for platforms supporting destructor attribute.
 *
 * This function is called when the library is unloaded.
 */
static void __attribute__ ((destructor)) libEnd(void)
{
  CloseDebugFiles();
}

#endif

#ifdef _MANAGED
  #pragma managed(pop)
#endif

