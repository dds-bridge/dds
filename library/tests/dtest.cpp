/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <iostream>
#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
  #include <unistd.h>
#endif

#include <api/dll.h>
#include <system/parallel_boards.hpp>
#include "testcommon.hpp"
#include "args.hpp"
#include "cst.hpp"

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>

EM_JS(void, dtest_schedule_pthread_clean_exit, (int code), {
  var exitCode = code;
  var attempts = 100;
  var finish = function () {
    if (typeof PThread !== 'undefined') {
      var workers = [].concat(
        PThread.unusedWorkers || [],
        PThread.runningWorkers || []);
      var pending = workers.some(function (w) { return !w.loaded; });
      if (pending && attempts-- > 0) {
        setTimeout(finish, 0);
        return;
      }
      if (PThread.terminateAllThreads)
        PThread.terminateAllThreads();
    }
    if (typeof process !== 'undefined')
      process.exit(exitCode);
  };
  // Node processes Worker messages before setImmediate callbacks.
  if (typeof setImmediate === 'function')
    setImmediate(finish);
  else
    setTimeout(finish, 0);
});

// Join C++ workers, let pending Worker "loaded" messages flush, then tear down
// Emscripten pthread Workers and exit Node. Avoids ASSERTIONS noise of the form
// `received "loaded" command from terminated worker` when -n exceeds the
// precreated PTHREAD_POOL_SIZE.
[[noreturn]] static void dtest_emscripten_clean_exit(const int code)
{
  dds::internal::shutdown_parallel_boards_pool();
  dtest_schedule_pthread_clean_exit(code);
  emscripten_exit_with_live_runtime();
}
#endif


using std::cout;
using std::endl;

OptionsType options;


int main(int argc, char * argv[])
{
  read_args(argc, argv);

  SetResources(options.memory_mb_, 0);

  DDSInfo info;
  GetDDSInfo(&info);
  cout << info.systemString << endl;
  if (options.num_threads_ == 0)
    cout << "dtest worker threads: auto\n";
  else
    cout << "dtest worker threads: " << options.num_threads_ << "\n";

  real_main(argc, argv);

#if defined(__EMSCRIPTEN__)
  dtest_emscripten_clean_exit(0);
#else
  // Restore normal termination so destructors / atexit handlers run.
  exit(0);
#endif
}
