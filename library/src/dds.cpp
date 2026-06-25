/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include <init.hpp>
#include <api/dll.h>

#if defined(_WIN32) || defined(USES_DLLMAIN)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
#endif

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
    InitializeStaticMemory();
  else if (ul_reason_for_call == DLL_PROCESS_DETACH)
  {
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
  InitializeStaticMemory();
}


/**
 * @brief Finalize and clean up the DDS library.
 */
void DDSFinalize(void)
{
  FreeMemory();
}


/**
 * @brief Library constructor/destructor for Apple platforms.
 *
 * Register DDSInitialize/DDSFinalize so the library's static memory is set
 * up automatically when the library is loaded, matching the behaviour of the
 * Windows (DllMain) and USES_CONSTRUCTOR paths. This frees callers from having
 * to call InitializeStaticMemory() themselves.
 */
static void __attribute__ ((constructor)) libInit(void)
{
  DDSInitialize();
}


static void __attribute__ ((destructor)) libFini(void)
{
  DDSFinalize();
}

#elif defined(USES_CONSTRUCTOR)

/**
 * @brief Library constructor for platforms supporting constructor attribute.
 *
 * This function is called when the library is loaded.
 */
static void __attribute__ ((constructor)) libInit(void)
{
  InitializeStaticMemory();
}

#endif

#ifdef _MANAGED
  #pragma managed(pop)
#endif

