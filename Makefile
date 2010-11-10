GPP = g++
#GPP = clang++
CXXFLAGS = -std=c++98 -Wall -pedantic -fvisibility=hidden -fno-exceptions -fno-rtti
OPT_FLAGS = -DNDEBUG -g -fomit-frame-pointer
DEBUG_FLAGS = -DDEBUG -O0 -ggdb -gdwarf-4 -fno-omit-frame-pointer
PGO_GEN_FLAGS = -fprofile-generate -O3
PGO_USE_FLAGS = -fprofile-use -O3
ifeq (${DEBUG}, yes)
FLAGS = $(CXXFLAGS) $(DEBUG_FLAGS)
else ifeq (${PGO_GEN}, yes)
FLAGS = $(CXXFLAGS) $(OPT_FLAGS) $(PGO_GEN_FLAGS)
else ifeq (${PGO_USE}, yes)
FLAGS = $(CXXFLAGS) $(OPT_FLAGS) $(PGO_USE_FLAGS)
else
FLAGS = $(CXXFLAGS) $(OPT_FLAGS) -Os
endif

LDFLAGS = -Wl,-O1 -fwhole-program

all: lightbot_solver

lightbot_solver.o: lightbot_solver.cpp
	$(GPP) $(FLAGS) -c -o $@ $<

lightbot_solver: lightbot_solver.o
	$(GPP) $(FLAGS) $(LDFLAGS) -o $@ $<

lightbot_solver_pgo: lightbot_solver.cpp
	rm -f lightbot_solver lightbot_solver.o
	$(MAKE) lightbot_solver PGO_GEN=yes
	./lightbot_solver 100 100 &>/dev/null
	rm -f lightbot_solver lightbot_solver.o
	$(MAKE) lightbot_solver PGO_USE=yes

clean:
	rm -f perf.data perf.data.old *.o *.gcda cachegrind.out.* \
	lightbot_solver
