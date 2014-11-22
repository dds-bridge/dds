void InitGame(
  int			gameNo,
  bool			moveTreeFlag,
  int			first,
  int			handRelFirst,
  int			thrId);

void InitSearch(
  struct pos		* posPoint,
  int			depth,
  struct moveType	startMoves[],
  int			first,
  bool			mtd,
  int			thrId);

double ThreadMemoryUsed();

void CloseDebugFiles();

// Used by SH for stand-alone mode.
void DDSidentify(char * s);
