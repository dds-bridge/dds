# This is a Makefile for the examples,
# for Linux and the GNU g++ compiler.

# It does assume a Unix-like setup for some commands,
# but if you only want to call "make" with the default target,
# you should be OK.

# If your compiler name is not given here, change it.
CC		= g++

CC_FLAGS	= -O3 -flto -mtune=generic -fopenmp

# These flags are not turned on by default, but DDS should pass them.
# Turn them on below.
WARN_FLAGS	= 		\
	-Wshadow 		\
	-Wsign-conversion 	\
	-pedantic -Wall -Wextra  \
	-Wcast-align -Wcast-qual \
	-Wctor-dtor-privacy 	\
	-Wdisabled-optimization \
	-Winit-self 		\
	-Wlogical-op		\
	-Wmissing-declarations 	\
	-Wmissing-include-dirs 	\
	-Wnoexcept 		\
	-Wold-style-cast 	\
	-Woverloaded-virtual 	\
	-Wredundant-decls 	\
	-Wsign-promo 		\
	-Wstrict-null-sentinel	\
	-Wstrict-overflow=1 	\
	-Wswitch-default -Wundef \
	-Werror 		\
	-Wno-unused 		\
	-Wno-unknown-pragmas 	\
	-Wno-long-long		\
	-Wno-format

# Here you can turn on warnings.
# CC_FULL_FLAGS	= $(CC_FLAGS)
CC_FULL_FLAGS	= $(CC_FLAGS) $(WARN_FLAGS)

DLLBASE		= dds
STATIC_LIB	= lib$(DLLBASE).a

COMMON_SOURCE_FILES 	=	\
	hands.cpp

ALL_EXAMPLE_FILES	=	\
	AnalysePlayBin		\
	AnalysePlayPBN		\
	AnalyseAllPlaysBin	\
	AnalyseAllPlaysPBN	\
	CalcDDtable.cpp		\
	CalcDDtablePBN.cpp	\
	CalcAllTables.cpp	\
	CalcAllTablesPBN.cpp	\
	DealerPar.cpp		\
	Par.cpp			\
	SolveBoard.cpp		\
	SolveBoardPBN.cpp	\
	SolveAllBoards.cpp

LIB_FLAGS	= -L. -l$(DLLBASE)

LD_FLAGS	= 

OBJ_FILES	= $(subst .cpp,.o,$(COMMON_SOURCE_FILES))
EX_OBJ_FILES	= $(subst .cpp,.o,$(ALL_EXAMPLE_FILES))
EX_EXE_FILES	= $(subst .cpp,,$(ALL_EXAMPLE_FILES))

AnalysePlayBin:	$(OBJ_FILES) AnalysePlayBin.o
	$(CC) $(CC_FULL_FLAGS) $(LD_FLAGS) $(OBJ_FILES) AnalysePlayBin.o $(LIB_FLAGS) -o AnalysePlayBin

AnalysePlayPBN:	$(OBJ_FILES) AnalysePlayPBN.o
	$(CC) $(CC_FULL_FLAGS) $(LD_FLAGS) $(OBJ_FILES) AnalysePlayPBN.o $(LIB_FLAGS) -o AnalysePlayPBN

AnalyseAllPlaysBin:	$(OBJ_FILES) AnalyseAllPlaysBin.o
	$(CC) $(CC_FULL_FLAGS) $(LD_FLAGS) $(OBJ_FILES) AnalyseAllPlaysBin.o $(LIB_FLAGS) -o AnalyseAllPlaysBin

AnalyseAllPlaysPBN:	$(OBJ_FILES) AnalyseAllPlaysPBN.o
	$(CC) $(CC_FULL_FLAGS) $(LD_FLAGS) $(OBJ_FILES) AnalyseAllPlaysPBN.o $(LIB_FLAGS) -o AnalyseAllPlaysPBN

CalcDDtable:	$(OBJ_FILES) CalcDDtable.o
	$(CC) $(CC_FULL_FLAGS) $(LD_FLAGS) $(OBJ_FILES) CalcDDtable.o $(LIB_FLAGS) -o CalcDDtable

CalcDDtablePBN:	$(OBJ_FILES) CalcDDtablePBN.o
	$(CC) $(CC_FULL_FLAGS) $(LD_FLAGS) $(OBJ_FILES) CalcDDtablePBN.o $(LIB_FLAGS) -o CalcDDtablePBN

CalcAllTables:	$(OBJ_FILES) CalcAllTables.o
	$(CC) $(CC_FULL_FLAGS) $(LD_FLAGS) $(OBJ_FILES) CalcAllTables.o $(LIB_FLAGS) -o CalcAllTables

CalcAllTablesPBN:	$(OBJ_FILES) CalcAllTablesPBN.o
	$(CC) $(CC_FULL_FLAGS) $(LD_FLAGS) $(OBJ_FILES) CalcAllTablesPBN.o $(LIB_FLAGS) -o CalcAllTablesPBN

DealerPar:	$(OBJ_FILES) DealerPar.o
	$(CC) $(CC_FULL_FLAGS) $(LD_FLAGS) $(OBJ_FILES) DealerPar.o $(LIB_FLAGS) -o DealerPar

Par:	$(OBJ_FILES) Par.o
	$(CC) $(CC_FULL_FLAGS) $(LD_FLAGS) $(OBJ_FILES) Par.o $(LIB_FLAGS) -o Par

SolveBoard:	$(OBJ_FILES) SolveBoard.o
	$(CC) $(CC_FULL_FLAGS) $(LD_FLAGS) $(OBJ_FILES) SolveBoard.o $(LIB_FLAGS) -o SolveBoard

SolveBoardPBN:	$(OBJ_FILES) SolveBoardPBN.o
	$(CC) $(CC_FULL_FLAGS) $(LD_FLAGS) $(OBJ_FILES) SolveBoardPBN.o $(LIB_FLAGS) -o SolveBoardPBN

SolveAllBoards:	$(OBJ_FILES) SolveAllBoards.o
	$(CC) $(CC_FULL_FLAGS) $(LD_FLAGS) $(OBJ_FILES) SolveAllBoards.o $(LIB_FLAGS) -o SolveAllBoards

%.o:	%.cpp
	$(CC) $(CC_FULL_FLAGS) -c $< -o $*.o

depend:
	makedepend -Y -- $(cOMMON_SOURCE_FILES) $(ALL_EXAMPLE_FILES)

clean:
	rm -f $(OBJ_FILES) $(EX_OBJ_FILES) $(EX_EXE_FILES) $(STATIC_LIB)

