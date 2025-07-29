# --------------------- INFORMATION --------------------------------

# This the test Makefile for Windows and the Cygwin GNU g++
# compiler.  It assumes a Unix-like setup for some commands.

# The dtest program itself does not use multi-threading, 
# but the library might, depending on how it was compiled.

# The Makefile also allows an "un-official" and ugly, but
# sometimes practical compilation of a directly integrated
# executable (i.e. not using the DLL).  For this the Makefile
# uses the source and object files in the src directory...
# Use "make itest" at your own risk.

# If you need to add something for the threading system, this is
# the place.

CC_BOOST_LINK	= -lboost_system -lboost_thread

THREAD_LINK	= $(CC_BOOST_LINK)

# ----------------------- OFTEN OK    ------------------------------

# From here on you you don't have to change anything to CONFIGURE
# the compilation.  But you may well have to change something to 
# get it to compile.

INCL_DDS_SOURCE	= Makefiles/dds_sources.txt
INCL_OWN_SOURCE	= Makefiles/own_sources.txt
INCL_DEPENDS	= Makefiles/depends_o.txt

# If your compiler name is not given here, change it.
CC		= g++

# We compile with aggressive warnings, but we have to turn off some
# of them as they appear in libraries in great numbers...

WARN_FLAGS	= 		\
	-Wshadow 		\
	-Wsign-conversion 	\
	-pedantic -Wall -Wextra  \
	-Wcast-align -Wcast-qual \
	-Wctor-dtor-privacy 	\
	-Wdisabled-optimization \
	-Wformat=2 		\
	-Winit-self 		\
	-Wlogical-op 		\
	-Wmissing-declarations 	\
	-Wmissing-include-dirs 	\
	-Wnoexcept 		\
	-Wold-style-cast 	\
	-Woverloaded-virtual 	\
	-Wredundant-decls 	\
	-Wsign-promo 		\
	-Wstrict-null-sentinel 	\
	-Wstrict-overflow=1 	\
	-Wswitch-default -Wundef \
	-Werror 		\
	-Wno-unused 		\
	-Wno-unknown-pragmas 	\
	-Wno-long-long

COMPILE_FLAGS	= -O3 -flto -fopenmp -mtune=generic \
		-fno-use-linker-plugin \
		$(WARN_FLAGS)

DLLBASE		= dds
DLL 		= $(DLLBASE).dll
DLIB 		= $(DLLBASE).lib
EXPORTER	= Exports.def

LINK1_FLAGS	= 
LINK2_FLAGS	= 		\
	-Wl,--subsystem,windows \
	-Wl,--output-def,$(DLLBASE).def	\
	-Wl,--dynamicbase 	\
	-Wl,--nxcompat 		\
	-Wl,--no-seh 		\
	-Wl,--enable-stdcall-fixup \
	$(THREAD_LINK) \
	-L. -l$(DLLBASE)

# This is in addition to $(DTEST).cpp

include $(INCL_OWN_SOURCE)

DTEST_OBJ_FILES	= $(subst .cpp,.o,$(DTEST_SOURCE_FILES)) $(DTEST).o

DTEST		= dtest
ITEST		= itest

# These are the files that we steal from the src directory.

include $(INCL_DDS_SOURCE)

ITEST_SOURCE_FILES	=	\
	$(DDS_SOURCE_FILES)	\
	$(DTEST_SOURCE_FILES)	\
	itest.cpp

ITEST_OBJ_FILES	= $(subst .cpp,.o,$(ITEST_SOURCE_FILES))

dtest:	$(DTEST_OBJ_FILES)
	$(CC) $(COMPILE_FLAGS) $(LINK1_FLAGS) $(DTEST_OBJ_FILES) \
	$(LINK2_FLAGS) -o $(DTEST)

itest:	$(ITEST_OBJ_FILES)
	$(CC) $(COMPILE_FLAGS) $(LINK1_FLAGS) $(ITEST_OBJ_FILES) \
	$(LINK2_FLAGS) -o $(ITEST)

%.o:	%.cpp
	$(CC) $(COMPILE_FLAGS) -c $< -o $*.o

depend:
	makedepend -Y -- $(ITEST_SOURCE_FILES) $(DTEST).cpp

clean:
	rm -f $(ITEST_OBJ_FILES) $(DTEST).{o,exe} $(ITEST).exe \
	$(DLL) $(DLLBASE).def

# If you don't have a Linux-like setup, use "del" instead of "rm".

include $(INCL_DEPENDS)

