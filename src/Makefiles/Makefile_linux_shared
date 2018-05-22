# --------------------- INFORMATION --------------------------------

# This the DDS Makefile for Linux and the GNU g++ compiler.
# It creates a dynamically linked (shared) library, libdds.so.

# --------------------- CONFIGURATION ------------------------------

# You can configure the following:

# 1. The threading systems that you want in the library.
# You will always get single-threading.  If you have multiple
# threading systems, the default will be the multi-threading one
# with the lowest number (see System.cpp).  All that matters is
# CC_THREADING.

# GCD and WINAPI don't work on Windows.
THR_BOOST	= -DDDS_THREADS_BOOST
THR_GCD		= -DDDS_THREADS_GCD
THR_OPENMP	= -DDDS_THREADS_OPENMP
THR_WINAPI	= -DDDS_THREADS_WINAPI
THR_STL		= -DDDS_THREADS_STL

THREADING	= $(THR_BOOST) $(THR_OPENMP) $(THR_STL)

# If you need to add something for a threading system, this is
# the place.

CC_BOOST_LINK	= -lboost_system -lboost_thread

THREAD_COMPILE	= -fopenmp
THREAD_LINK	= -fopenmp $(CC_BOOST_LINK)

# 2. Debugging options.  (There are more granular options in debug.h.)

DEBUG_ALL	= -DDDS_DEBUG_ALL 
TIMING		= -DDDS_TIMING
SCHEDULER	= -DDDS_SCHEDULER

# All that matters from no. 2 and no. 3 is the following.  Here you
# can add $(SMALL_MEMORY) etc.

DDS_BEHAVIOR	=

# ----------------------- OFTEN OK    ------------------------------

# From here on you you don't have to change anything to CONFIGURE
# the compilation.  But you may well have to change something to 
# get it to compile.

INCL_SOURCE	= Makefiles/sources.txt
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
	-Wno-long-long		\
	-Wno-format

COMPILE_FLAGS	= -fPIC -O3 -flto -fopenmp -mtune=generic -std=c++11 \
		$(WARN_FLAGS) \
		$(DDS_BEHAVIOR) $(THREAD_COMPILE) $(THREADING)

DLLBASE		= dds
SHARED_LIB	= lib$(DLLBASE).so

include $(INCL_SOURCE)

O_FILES 	= $(subst .cpp,.o,$(SOURCE_FILES))

LINK_FLAGS      =               \
        -shared                 \
        -Wl,-O2                 \
        -Wl,--sort-common       \
        -Wl,--as-needed         \
        -Wl,-z                  \
        -Wl,relro               \
        $(THREAD_LINK)          \
        -fPIC


linux:	$(O_FILES)
	$(CC) \
        -o $(SHARED_LIB) $(O_FILES) $(LINK_FLAGS)

%.o:	%.cpp
	$(CC) $(COMPILE_FLAGS) -c $<

depend:
	makedepend -Y -- $(SOURCE_FILES)

clean:
	rm -f $(O_FILES) $(SHARED_LIB)

install:
	test -d ../test || mkdir ../test
	test -d ../examples || mkdir ../examples
	cp $(STATIC_LIB) ../test
	cp $(STATIC_LIB) ../examples

include $(INCL_DEPENDS)

