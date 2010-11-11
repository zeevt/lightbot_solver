GPP = g++
#GPP = clang++
LDFLAGS = -Wl,-O1 -fwhole-program -flto
CXXFLAGS = -std=c++98 -Wall -pedantic -fvisibility=hidden -fno-exceptions -fno-rtti -flto
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


all: lightbot_solver curses_player

%.o: %.cpp lightbot_solver.h
	$(GPP) $(FLAGS) -c -o $@ $<

lightbot_solver: lightbot_solver.o curses_player.o
	$(GPP) $(FLAGS) $(LDFLAGS) -o $@ $^ -lrt -lncurses

lightbot_solver_pgo: lightbot_solver.cpp
	rm -f lightbot_solver lightbot_solver.o
	$(MAKE) lightbot_solver PGO_GEN=yes
	./lightbot_solver 100 100 &>/dev/null
	rm -f lightbot_solver lightbot_solver.o
	$(MAKE) lightbot_solver PGO_USE=yes

curses_player: curses_player.cpp
	$(GPP) $(FLAGS) -DTEST -o $@ $< -lncurses

clean:
	rm -f perf.data perf.data.old *.o *.gcda cachegrind.out.* \
	lightbot_solver curses_player
