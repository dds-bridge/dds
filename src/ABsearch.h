#define DDS_POS_LINES	5
#define DDS_HAND_LINES 	12
#define DDS_NODE_LINES	4
#define DDS_FULL_LINE	80
#define DDS_HAND_OFFSET	16
#define DDS_HAND_OFFSET2 12
#define DDS_DIAG_WIDTH  34


bool ABsearch(
  struct pos 		* posPoint, 
  int 			target, 
  int 			depth, 
  struct localVarType 	* thrp);

bool ABsearch0(
  struct pos		* posPoint,
  int			target,
  int			depth,
  struct localVarType	* thrp);

bool ABsearch1(
  struct pos		* posPoint,
  int			target,
  int			depth,
  struct localVarType	* thrp);

bool ABsearch2(
  struct pos		* posPoint,
  int			target,
  int			depth,
  struct localVarType	* thrp);

bool ABsearch3(
  struct pos		* posPoint,
  int			target,
  int			depth,
  struct localVarType	* thrp);

void InitFileTopLevel(
  int			thrId);

void InitFileABstats(
  int			thrId);

void InitFileABhits(
  int			thrId);

void InitFileTTstats(
  int			thrId);

void InitFileTimer(
  int			thrId);

void CloseFileTopLevel(
  localVarType		* thrp);

void CloseFileABhits(
  localVarType		* thrp);

void DumpTopLevel(
  struct localVarType	* thrp,
  int			tricks,
  int			lower,
  int			upper,
  int			printMode);

