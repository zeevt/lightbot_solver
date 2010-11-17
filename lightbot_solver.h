#ifndef lightbot_solver_h_
#define lightbot_solver_h_

enum direction { N, E, S, W };

static const char *CMD_CHARS = "RL12*F^_";
enum cmd_e {
  right,
  left,
  f1,
  f2,
  light,
  forward,
  jump,
  nop
};

#define CMDS_IN_MAIN 12
#define CMDS_IN_FUNC 8
#define F1_START     12
#define F2_START     20
#define CMDS_IN_PRG  28

struct program_t { uint8_t cmds[CMDS_IN_PRG]; };

class square
{
private:
  uint8_t c;
public:
  uint8_t get_height(void) const { return c & 63; }
  int has_light(void) const { return (c & 128) >> 7; }
  int is_lit(void) const { return (c & 64) >> 6; }
  void switch_light(void) { c = (((c >> 1) ^ c) & 64) | (c & ~64); }
  void reset_light(void) { c &= ~64; }
};

struct coord_t { uint8_t x, y; };

struct player_funcs_t
{
  struct state_t* (*init)(
    int map_height,
    int map_width,
    const square *map,
    const struct program_t *prg);
  int (*shutdown)(struct state_t *state);
  void (*set_pc)(struct state_t *state, uint8_t pc);
  void (*set_coord)(struct state_t *state, struct coord_t coord);
  void (*set_dir)(struct state_t *state, enum direction dir);
  void (*switch_light)(struct state_t *state);
};

#endif /* lightbot_solver_h_ */
