// ThreadData instance methods implementation
#include "thread_data.hpp"
#include <utility/debug.h>

using std::string;

void ThreadData::init_debug_files([[maybe_unused]] const string& suffix)
{
  if (debug_files_initialized_)
    return;

#ifdef DDS_TOP_LEVEL
	fileTopLevel.SetName(DDS_TOP_LEVEL_PREFIX + suffix);
#endif

#ifdef DDS_AB_STATS
	fileABstats.SetName(DDS_AB_STATS_PREFIX + suffix);
#endif

#ifdef DDS_AB_HITS
	fileRetrieved.SetName(DDS_AB_HITS_RETRIEVED_PREFIX + suffix);
	fileStored.SetName(DDS_AB_HITS_STORED_PREFIX + suffix);
#endif

#ifdef DDS_TT_STATS
	fileTTstats.SetName(DDS_TT_STATS_PREFIX + suffix);
#endif

#ifdef DDS_TIMING
	fileTimerList.SetName(DDS_TIMING_PREFIX + suffix);
#endif

#ifdef DDS_MOVES
	fileMoves.SetName(DDS_MOVES_PREFIX + suffix);
#endif

  debug_files_initialized_ = true;
}

void ThreadData::close_debug_files()
{
  if (!debug_files_initialized_)
    return;

#ifdef DDS_TOP_LEVEL
	fileTopLevel.Close();
#endif

#ifdef DDS_AB_STATS
	fileABstats.Close();
#endif

#ifdef DDS_AB_HITS
	fileRetrieved.Close();
	fileStored.Close();
#endif

#ifdef DDS_TT_STATS
	fileTTstats.Close();
#endif

#ifdef DDS_TIMING
	fileTimerList.Close();
#endif

#ifdef DDS_MOVES
	fileMoves.Close();
#endif

  debug_files_initialized_ = false;
}
