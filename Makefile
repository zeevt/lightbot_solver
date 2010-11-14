ifeq ($(CC),icc)
GPP = icpc
LDFLAGS = -Wl,-O1 -Wl,--as-needed
CXXFLAGS = -std=gnu++98 -strict-ansi -fno-math-errno -Wall -pipe -fvisibility=hidden -fno-exceptions -fno-rtti
OPT_FLAGS = -DNDEBUG -g -gdwarf-2 -mssse3 -mtune=core2 -fomit-frame-pointer -ansi-alias
DEBUG_FLAGS = -DDEBUG -O0 -ggdb -gdwarf-4 -fno-omit-frame-pointer
PGO_GEN_FLAGS = -prof-gen -O3
PGO_USE_FLAGS = -prof-use -O3
else ifeq ($(CC), clang)
GPP = clang++
LDFLAGS = -Wl,-O1 -fwhole-program
CXXFLAGS = -std=c++98 -Wall -pedantic -fvisibility=hidden -fno-exceptions -fno-rtti
OPT_FLAGS = -DNDEBUG -g -fomit-frame-pointer
DEBUG_FLAGS = -DDEBUG -O0 -ggdb -gdwarf-4 -fno-omit-frame-pointer
else
GPP = g++
LDFLAGS = -Wl,-O1 -Wl,--as-needed -fwhole-program -flto
CXXFLAGS = -std=c++98 -Wall -pedantic -fvisibility=hidden -fno-exceptions -fno-rtti -flto
OPT_FLAGS = -DNDEBUG -g -fomit-frame-pointer
DEBUG_FLAGS = -DDEBUG -O0 -ggdb -gdwarf-4 -fno-omit-frame-pointer
PGO_GEN_FLAGS = -fprofile-generate -O3
PGO_USE_FLAGS = -fprofile-use -O3
endif


ifeq (${DEBUG}, yes)
FLAGS = $(CXXFLAGS) $(DEBUG_FLAGS)
else ifeq (${PGO_GEN}, yes)
FLAGS = $(CXXFLAGS) $(OPT_FLAGS) $(PGO_GEN_FLAGS)
else ifeq (${PGO_USE}, yes)
FLAGS = $(CXXFLAGS) $(OPT_FLAGS) $(PGO_USE_FLAGS)
else
FLAGS = $(CXXFLAGS) $(OPT_FLAGS) -O2
endif


all: lightbot_solver curses_player

%.o: %.cpp lightbot_solver.h
	$(GPP) $(FLAGS) -c -o $@ $<

lightbot_solver: lightbot_solver.o curses_player.o jit.o
	$(GPP) $(FLAGS) $(LDFLAGS) -finstrument-functions-exclude-function-list=do_right,do_left,do_light,do_before_step,do_forward,do_jump -o $@ $^ -lrt -lncurses

lightbot_solver_pgo: lightbot_solver.cpp jit.o
	rm -f lightbot_solver lightbot_solver.o curses_player.o
	$(MAKE) lightbot_solver CC=$(CC) PGO_GEN=yes
	./lightbot_solver out.temp 100 100 &>/dev/null
	rm -f lightbot_solver lightbot_solver.o curses_player.o
	$(MAKE) lightbot_solver CC=$(CC) PGO_USE=yes

curses_player: curses_player.cpp
	$(GPP) $(FLAGS) -DTEST -o $@ $< -lncurses

jit: jit.cpp lightbot_solver.h
	$(GPP) $(FLAGS) $(LDFLAGS) -DTEST -o $@ jit.cpp

clean:
	rm -f perf.data perf.data.old *.o *.gcda cachegrind.out.* \
	*.temp *.dyn *.dpi *.dpi.lock \
	lightbot_solver curses_player jit
