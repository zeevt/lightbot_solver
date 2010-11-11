#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <ncurses.h>
#include "lightbot_solver.h"

struct state_t
{
  int map_height, map_width;
  WINDOW *cmd_window, *map_window;
  const square *map;
  const struct program_t *prg;
  struct coord_t coord;
  uint8_t pc;
  enum direction dir;
};

static int arrows[4];

static void init_ncurses()
{
  initscr();
  if (has_colors() == FALSE)
  {
    endwin();
    printf("Your terminal does not support color\n");
    exit(1);
  }
  curs_set(0);
  cbreak();
  noecho();
  start_color();
  init_pair(1, COLOR_YELLOW, COLOR_BLACK);
  init_pair(2, COLOR_BLUE, COLOR_BLACK);
  init_pair(3, COLOR_CYAN, COLOR_BLACK);
  init_pair(4, COLOR_WHITE, COLOR_BLACK);
}

static void draw_program(struct state_t *state)
{
  for (int row = 0; row < 7; row++)
  {
    for (int col = 0; col < 4; col++)
    {
      int pc = row * 4 + col;
      assert((pc >= 0) && (pc < 28));
      uint8_t cmd = state->prg->cmds[pc];
      assert((cmd >= 0) && (cmd < 8));
      char c = CMD_CHARS[cmd];
      if (pc == state->pc)
      {
        mvwaddch(state->cmd_window, row, col * 2,
                 c | A_BOLD | COLOR_PAIR(1));
      }
      else
      {
        mvwaddch(state->cmd_window, row, col * 2,
                 c | COLOR_PAIR(3));
      }
    }
  }
  wrefresh(state->cmd_window);
}

static void draw_map(struct state_t *state)
{
  for (int row = 0; row < state->map_height; row++)
  {
    for (int col = 0; col < state->map_width; col++)
    {
      int map_idx = row * state->map_width + col;
      const square *cell = &state->map[map_idx];
      const char c = cell->get_height() + 48;
      if (cell->has_light())
        if (cell->is_lit())
          mvwaddch(state->map_window, row, col * 2, c | A_BOLD | COLOR_PAIR(1));
        else
          mvwaddch(state->map_window, row, col * 2, c | COLOR_PAIR(2));
      else
        mvwaddch(state->map_window, row, col * 2, c | COLOR_PAIR(3));
      if ((row == state->coord.y) && (col == state->coord.x))
      {
        assert((state->dir >= 0) && (state->dir < 4));
        const int c = arrows[state->dir];
        mvwaddch(state->map_window, row, col * 2 + 1, c | A_BOLD | COLOR_PAIR(4));
      }
    }
  }
  wrefresh(state->map_window);
}


#define max(a, b) ((a>b)?a:b)

struct state_t* player_init(
  int map_height,
  int map_width,
  const square *map,
  const struct program_t *prg)
{
  struct state_t* state = new struct state_t;
  init_ncurses();
  getmaxyx(stdscr, state->map_height, state->map_width);
  if ((state->map_height < max(map_height, 7)) || (state->map_width < map_width * 2 + 12 + 2))
  {
    delete state;
    return NULL;
  }
  state->map = map;
  state->prg = prg;
  state->pc = 0;
  state->coord.x = 0;
  state->coord.y = 0;
  state->dir = N;
  state->map_height = map_height;
  state->map_width = map_width;
  state->cmd_window = newwin(7, 12, 0, map_width * 2 + 2);
  state->map_window = newwin(map_height, map_width * 2, 0, 0);
  arrows[0] = ACS_DARROW; arrows[1] = ACS_RARROW;
  arrows[2] = ACS_UARROW; arrows[3] = ACS_LARROW;
  refresh();
  draw_program(state);
  draw_map(state);
  return state;
}

int player_shutdown(struct state_t *state)
{
  delwin(state->map_window);
  delwin(state->cmd_window);
  curs_set(1);
  nocbreak();
  echo();
  endwin();
  delete state;
#ifdef DEBUG
  state->map = NULL;
  state->prg = NULL;
  state->pc = 0;
  state->coord.x = 0;
  state->coord.y = 0;
  state->map_height = 0;
  state->map_width = 0;
  state->cmd_window = NULL;
  state->map_window = NULL;
#endif
  return 0;
}

void player_set_pc(struct state_t *state, uint8_t pc)
{
  state->pc = pc;
  draw_program(state);
  refresh();
  usleep(100000);
}

void player_set_coord(struct state_t *state, struct coord_t coord)
{
  mvwaddch(state->map_window, state->coord.y, state->coord.x * 2 + 1, ' ');
  state->coord = coord;
  draw_map(state);
  refresh();
  usleep(100000);
}

void player_set_dir(struct state_t *state, enum direction dir)
{
  state->dir = dir;
  draw_map(state);
  refresh();
  usleep(100000);
}

void player_switch_light(struct state_t *state)
{
  draw_map(state);
  refresh();
  usleep(100000);
}

extern const struct player_funcs_t ncurses_player =
{
  player_init,
  player_shutdown,
  player_set_pc,
  player_set_coord,
  player_set_dir,
  player_switch_light
};

#ifdef TEST

static void program_from_string(struct program_t* prg, const char *s)
{
  for (int char_idx = 0; char_idx < CMDS_IB_PRG; char_idx++)
  {
    for (int cmd_idx = 0; cmd_idx < 8; cmd_idx++)
    {
      if (s[char_idx] == CMD_CHARS[cmd_idx])
      {
        prg->cmds[char_idx] = cmd_idx;
        goto found;
      }
    }
    fprintf(stderr, "ERROR unknown command %c\n", s[char_idx]);
    found:;
  }
}

static const char* the_map =
  "\x00\x00\x00\x00\x00\x00\x00\x00"
  "\x00\x00\x00\x00\x01\x00\x00\x00"
  "\x82\x02\x02\x04\x02\x03\x02\x00"
  "\x00\x00\x00\x04\x03\x04\x02\x82"
  "\x00\x00\x00\x00\x00\x00\x00\x00";

int main()
{
  struct program_t prg;
  program_from_string(&prg, "1L^LFR21R2__FFF^L^^_^^FF*L^L");
  square map[5][8];
  memcpy(&map[0][0], the_map, sizeof(map));
  
  struct state_t* state;
  if ((state = player_init(5, 8, &map[0][0], &prg)) == NULL)
    goto out;
  
  refresh();
  getch();
out:
  player_shutdown(state);

  return 0;
}
#endif /* TEST */
