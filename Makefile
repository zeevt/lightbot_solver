ifeq ($(CC),icc)
GPP = icpc
LDFLAGS = -Wl,-O1 -Wl,--as-needed
CFLAGS = -std=gnu99 -Wall -pedantic
CXXFLAGS = -std=gnu++98 -strict-ansi -fno-math-errno -Wall -pipe -fvisibility=hidden -fno-exceptions -fno-rtti
OPT_FLAGS = -DNDEBUG -g -gdwarf-2 -mssse3 -mtune=core2 -fomit-frame-pointer -ansi-alias
DEBUG_FLAGS = -DDEBUG -O0 -ggdb -gdwarf-4 -fno-omit-frame-pointer
PGO_GEN_FLAGS = -prof-gen -O3
PGO_USE_FLAGS = -prof-use -O3
else ifeq ($(CC), clang)
GPP = clang++
LDFLAGS = -Wl,-O1 -fwhole-program
CFLAGS = -std=gnu99 -Wall -pedantic
CXXFLAGS = -std=c++98 -Wall -pedantic -fvisibility=hidden -fno-exceptions -fno-rtti
OPT_FLAGS = -DNDEBUG -g -fomit-frame-pointer
DEBUG_FLAGS = -DDEBUG -O0 -ggdb -gdwarf-4 -fno-omit-frame-pointer
else
CC = gcc
GPP = g++
LDFLAGS = -Wl,-O1 -Wl,--as-needed -fwhole-program -flto
CFLAGS = -std=gnu99 -Wall -pedantic
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

ifeq (${JIT},yes)
%.o: %.c
	$(CC) $(CFLAGS) $(OPT_FLAGS) -O2 -c -o $@ $<

%.o: %.cpp lightbot_solver.h
	$(GPP) $(FLAGS) -DJIT -c -o $@ $<

lightbot_solver: lightbot_solver.o curses_player.o jit.o jit_asm.o
	$(GPP) $(FLAGS) $(LDFLAGS) -o $@ $^ -lrt -lncurses

lightbot_solver_pgo: lightbot_solver.cpp jit_asm.o
	rm -f lightbot_solver lightbot_solver.o curses_player.o jit.o
	$(MAKE) lightbot_solver CC=$(CC) JIT=${JIT} PGO_GEN=yes
	./lightbot_solver out.temp 100 100 &>/dev/null
	rm -f lightbot_solver lightbot_solver.o curses_player.o jit.o
	$(MAKE) lightbot_solver CC=$(CC) JIT=${JIT} PGO_USE=yes
else
%.o: %.cpp lightbot_solver.h
	$(GPP) $(FLAGS) -c -o $@ $<

lightbot_solver: lightbot_solver.o curses_player.o
	$(GPP) $(FLAGS) $(LDFLAGS) -o $@ $^ -lrt -lncurses

lightbot_solver_pgo: lightbot_solver.cpp
	rm -f lightbot_solver lightbot_solver.o curses_player.o
	$(MAKE) lightbot_solver CC=$(CC) PGO_GEN=yes
	./lightbot_solver out.temp 100 100 &>/dev/null
	rm -f lightbot_solver lightbot_solver.o curses_player.o
	$(MAKE) lightbot_solver CC=$(CC) PGO_USE=yes
endif

curses_player: curses_player.cpp
	$(GPP) $(FLAGS) -DTEST -o $@ $< -lncurses

jit: jit.cpp jit.h lightbot_solver.h jit_asm.o
	$(GPP) $(FLAGS) $(LDFLAGS) -DTEST -o $@ jit.cpp jit_asm.o

clean:
	rm -f perf.data perf.data.old *.o *.gcda cachegrind.out.* \
	*.temp *.dyn *.dpi *.dpi.lock \
	lightbot_solver curses_player jit
