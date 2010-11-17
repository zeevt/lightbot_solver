#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
//#include <ncurses.h>
#include "unix_utils.h"
#include "lightbot_solver.h"
#ifdef JIT
#include "jit.h"
#endif

static const char* the_map =
  "\x00\x00\x00\x00\x00\x00\x00\x00"
  "\x00\x00\x00\x00\x01\x00\x00\x00"
  "\x82\x02\x02\x04\x02\x03\x02\x00"
  "\x00\x00\x00\x04\x03\x04\x02\x82"
  "\x00\x00\x00\x00\x00\x00\x00\x00";

static const uint8_t startX = 0, startY = 1;

static const enum direction startDirection = E;

/* http://groups.google.com/group/comp.lang.c/browse_thread/thread/a9915080a4424068/ */
/* http://www.jstatsoft.org/v08/i14/paper */
static uint32_t prng_state[] = {198765432,362436069,521288629,88675123,886756453};
static uint32_t xorshift(void)
{
  uint32_t t = prng_state[0]^(prng_state[0]>>7);
  prng_state[0]=prng_state[1]; prng_state[1]=prng_state[2]; prng_state[2]=prng_state[3]; prng_state[3]=prng_state[4];
  prng_state[4]=(prng_state[4]^(prng_state[4]<<6))^(t^(t<<13));
  return (prng_state[1]+prng_state[1]+1)*prng_state[4];
}

class prng_c
{
private:
  uint32_t state;
  uint8_t bits_left;
public:
  prng_c(void) : bits_left(0) { }
  uint32_t give_bits(uint8_t nbits)
  {
    if (bits_left < nbits)
    {
      state = xorshift();
      bits_left = 32;
    }
    uint32_t retval = state & ((1 << nbits) - 1);
    state >>= nbits;
    bits_left -= nbits;
    return retval;
  }
};


extern const struct player_funcs_t ncurses_player;


static void program_from_string(struct program_t* prg, const char *s)
{
  for (int char_idx = 0; char_idx < CMDS_IN_PRG; char_idx++)
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

static void program_rnd_fill(struct program_t* prg)
{
  prng_c prng;
  for (int i = 0; i < CMDS_IN_MAIN; i++)
    prg->cmds[i] = prng.give_bits(3);
  for (int i = F1_START; i < F2_START; i++)
    do
      prg->cmds[i] = prng.give_bits(3);
    while (prg->cmds[i] == f1);
  for (int i = F2_START; i < CMDS_IN_PRG; i++)
    do
      prg->cmds[i] = prng.give_bits(3);
    while ((prg->cmds[i] == f1) || (prg->cmds[i] == f2));
}

static void program_mutate(struct program_t* prg)
{
  prng_c prng;
  for (int mutation = prng.give_bits(3) + 1; mutation; mutation--)
  {
    int cmd_to_mutate = prng.give_bits(5);
    if (cmd_to_mutate >= CMDS_IN_PRG + 2)
    {
      /* insert instead of overwrite a cmd */
      cmd_to_mutate = prng.give_bits(5);
      if (cmd_to_mutate > CMDS_IN_PRG - 2)
        cmd_to_mutate -= CMDS_IN_PRG - 2;
      for (int i = CMDS_IN_PRG - 1; i > cmd_to_mutate; i--)
        prg->cmds[i] = prg->cmds[i - 1];
      if (prg->cmds[F1_START] == f1) prg->cmds[F1_START] = nop;
      if (prg->cmds[F2_START] == f2) prg->cmds[F2_START] = nop;
    }
    else if (cmd_to_mutate >= CMDS_IN_PRG)
    {
      /* delete a cmd, place new at end */
      cmd_to_mutate = prng.give_bits(5);
      if (cmd_to_mutate > CMDS_IN_PRG - 2)
        cmd_to_mutate -= CMDS_IN_PRG - 2;
      for (int i = cmd_to_mutate; i < CMDS_IN_PRG - 1; i++)
        prg->cmds[i] = prg->cmds[i + 1];
      cmd_to_mutate = CMDS_IN_PRG - 1;
    }
    if (cmd_to_mutate >= F2_START)
    {
      int new_cmd = prng.give_bits(3);
      if ((new_cmd == f1) || (new_cmd == f2)) new_cmd = nop;
      prg->cmds[cmd_to_mutate] = new_cmd;
    }
    else if (cmd_to_mutate >= F1_START)
    {
      int new_cmd = prng.give_bits(3);
      if (new_cmd == f1) new_cmd = nop;
      prg->cmds[cmd_to_mutate] = new_cmd;
    }
    else
    {
      int new_cmd = prng.give_bits(3);
      prg->cmds[cmd_to_mutate] = new_cmd;
    }
  }
}

static int __attribute__((unused)) program_is_valid(const struct program_t* prg)
{
  for (int i = 0; i < CMDS_IN_PRG; i++)
    assert(prg->cmds[i] >= 0 && prg->cmds[i] <= 7);
  for (int i = F1_START; i < F2_START; i++)
    if (prg->cmds[i] == f1)
      return 0;
  for (int i = F2_START; i < CMDS_IN_PRG; i++)
    if (prg->cmds[i] == f1 || prg->cmds[i] == f2)
      return 0;
  return 1;
}

static void program_print(const struct program_t* prg, FILE* stream)
{
  char temp[CMDS_IN_PRG];
  for (int i = 0; i < CMDS_IN_PRG; i++)
    temp[i] = CMD_CHARS[prg->cmds[i]];
  fwrite(temp, 1, CMDS_IN_MAIN, stream);
  putc(' ', stream);
  fwrite(temp+F1_START, 1, CMDS_IN_FUNC, stream);
  putc(' ', stream);
  fwrite(temp+F2_START, 1, CMDS_IN_FUNC, stream);
  putc('\n', stream);
}

static __attribute__((const))
int32_t min(int32_t a, int32_t b)
{
  return b + ((a-b) & (a-b)>>31);
}

static __attribute__((const))
int32_t max(int32_t a, int32_t b)
{
  return a - ((a-b) & (a-b)>>31);
}

static __attribute__((const))
struct coord_t step(const struct coord_t coord, enum direction dir)
{
  struct coord_t result = coord;
  switch(dir)
  {
    case N: result.y = min((int32_t)result.y + 1, 4); break;
    case E: result.x = min((int32_t)result.x + 1, 7); break;
    case S: result.y = max((int32_t)result.y - 1, 0); break;
    case W: result.x = max((int32_t)result.x - 1, 0); break;
  }
  return result;
}

template<bool callbacks>
static int program_execute(const struct program_t* prg,
                           square map[5][8],
                           const struct player_funcs_t* player)
{
  struct state_t *state = NULL;
  if (callbacks && player) state = player->init(5, 8, &map[0][0], prg);
  enum direction curr_dir = startDirection;
  if (callbacks && player) player->set_dir(state, curr_dir);
  struct coord_t curr = {startX, startY};
  if (callbacks && player) player->set_coord(state, curr);
  struct coord_t next;
  uint8_t curr_height, next_height;
  uint8_t return_stack[2];
  uint8_t max_height_reached = 0;
  uint8_t pc = 0;
  uint8_t *return_stack_top = &return_stack[0];
  do
  {
    again:
    /*getch();*/
    if (callbacks && player) player->set_pc(state, pc);
    switch (prg->cmds[pc])
    {
      case right:
        curr_dir = (enum direction)((curr_dir + 1) & 3);
        if (callbacks && player) player->set_dir(state, curr_dir);
        break;
      case left:
        curr_dir = (enum direction)((curr_dir + 3) & 3);
        if (callbacks && player) player->set_dir(state, curr_dir);
        break;
      case f1:
        *(return_stack_top++) = pc + 1;
        pc = F1_START;
        goto again;
      case f2:
        *(return_stack_top++) = pc + 1;
        pc = F2_START;
        goto again;
      case light:
        map[curr.y][curr.x].switch_light();
        if (callbacks && player) player->switch_light(state);
        break;
      case forward:
      case jump:
        next = step(curr, curr_dir);
        curr_height = map[curr.y][curr.x].get_height();
        next_height = map[next.y][next.x].get_height();
        if (prg->cmds[pc] == forward)
        {
          if (curr_height == next_height)
          {
            curr = next;
            if (callbacks && player) player->set_coord(state, curr);
          }
        }
        else if (next_height < curr_height)
        {
          curr = next;
          if (callbacks && player) player->set_coord(state, curr);
        }
        else if (next_height == curr_height + 1)
        {
          if (next_height > max_height_reached)
            max_height_reached = next_height;
          curr = next;
          if (callbacks && player) player->set_coord(state, curr);
        }
        break;
    }
    if (pc == F2_START - 1 || pc == CMDS_IN_PRG - 1)
      pc = *(--return_stack_top);
    else
      pc++;
  }
  while (pc != CMDS_IN_MAIN);
  if (callbacks && player) player->shutdown(state);
  int num_lights_lit = map[2][0].is_lit() + map[3][7].is_lit();
  return num_lights_lit << 8 | max_height_reached;
}

static void self_test()
{
  static const struct
  {
    const char* prg;
    uint8_t lights_lit, height_reached;
  }
  test_cases[] =
  {   /*           1       2      */
    { "1L^LFR21R2__FFF^L^^_^^FF*L^L", 2, 4 },
    { "**1**11_112*_*L_2^_2F_RF_^FL", 2, 4 }
  };
  square map[5][8];
  memcpy(&map[0][0], the_map, sizeof(map));
  for (unsigned test_idx = 0; test_idx < sizeof(test_cases)/sizeof(test_cases[0]); test_idx++)
  {
    struct program_t prg;
    program_from_string(&prg, test_cases[test_idx].prg);
    map[2][0].reset_light();
    map[3][7].reset_light();
    int result = program_execute<true>(&prg, map, &ncurses_player);
    int lights_lit = result >> 8;
    int height_reached = result & 255;
    if ((lights_lit != test_cases[test_idx].lights_lit) ||
        (height_reached != test_cases[test_idx].height_reached))
    {
      program_print(&prg, stderr);
      fprintf(stderr, "%d %d\n", lights_lit, height_reached);
      fprintf(stderr, "Test failed.\n");
    }
  }
}

struct stack_item {
  struct stack_item* next;
  int mutate_cnt;
  int result;
  struct program_t prg;
};

int main(int argc, char** argv)
{
  if (argc < 2)
  {
    fprintf(stderr,
            "Usage: %s output_file <num_rnd_tries> <max_mutate_cnt>\n",
            argv[0]);
    exit(1);
  }
//  self_test();
  int num_rnd_tries = 100000;
  int max_mutate_cnt = 10000;
  FILE* stream = fopen(argv[1], "w");
  if (!stream)
  {
    fprintf(stderr, "Couldn't open %s for writing.\n", argv[1]);
    exit(2);
  }
  if (argc > 2)
    num_rnd_tries = atoi(argv[2]);
  if (argc > 3)
    max_mutate_cnt = atoi(argv[3]);
  square map[5][8];
  memcpy(&map[0][0], the_map, sizeof(map));
#ifdef JIT
  JITter jitter;
#endif
  struct stack_item* top = new stack_item();
  top->next = NULL;
  top->mutate_cnt = 0;
  top->result = 0;
  int prev_result = 0;
  long exec_counter = 0;
  timestamp_t t0;
#ifdef TIMING
  timestamp_t mutating, generating, executing;
  timestamp_clear(mutating);
  timestamp_clear(generating);
  timestamp_clear(executing);
#endif
  init_timestamper();
  get_timestamp(t0);
  for (;;)
  {
#ifdef TIMING
    timestamp_t bt0, bt1;
#endif
    if (!top->next)
    {
      if (!--num_rnd_tries) break;
      program_rnd_fill(&top->prg);
      prev_result = 0;
    }
    else
    {
      if (top->next->mutate_cnt < max_mutate_cnt)
      {
#ifdef TIMING
        get_timestamp(bt0);
#endif
        top->prg = top->next->prg;
        program_mutate(&top->prg);
#ifdef TIMING
        get_timestamp(bt1);
        timestamp_diff(bt0, bt1);
        timestamp_add(mutating, bt0);
#endif
        top->next->mutate_cnt++;
      }
      else
      {
        struct stack_item *next = top->next->next;
        delete top->next;
        top->next = next;
        top->mutate_cnt = 0;
        if (top->next)
          prev_result = top->next->result;
        continue;
      }
    }
    exec_counter = (exec_counter + 1) & ((1 << 20) - 1);
    if (!exec_counter)
    {
      timestamp_t t1;
      get_timestamp(t1);
      timestamp_diff(t0,t1);
      printf("%d program executions took " PRINTF_TIMESTAMP_STR " sec.\n",
             1 << 20, PRINTF_TIMESTAMP_VAL(t0));
#ifdef TIMING
      printf("time mutating:\t" PRINTF_TIMESTAMP_STR " sec.\n",
             PRINTF_TIMESTAMP_VAL(mutating));
      printf("time generating code:\t" PRINTF_TIMESTAMP_STR " sec.\n",
             PRINTF_TIMESTAMP_VAL(generating));
      printf("time executing code:\t" PRINTF_TIMESTAMP_STR " sec.\n",
             PRINTF_TIMESTAMP_VAL(executing));
      timestamp_clear(mutating);
      timestamp_clear(generating);
      timestamp_clear(executing);
#endif
      fflush(stdout);
      t0 = t1;
    }
    map[2][0].reset_light();
    map[3][7].reset_light();
    int num_lights_lit;
#ifdef JIT
#ifdef TIMING
    get_timestamp(bt0);
#endif
    jitter.generate_code(&top->prg);
#ifdef TIMING
    get_timestamp(bt1);
    timestamp_diff(bt0, bt1);
    timestamp_add(generating, bt0);
    get_timestamp(bt0);
#endif
    int max_height_reached = jitter.run_program(1, 0, 1, &map[0][0]);
#ifdef TIMING
    get_timestamp(bt1);
    timestamp_diff(bt0, bt1);
    timestamp_add(executing, bt0);
#endif
    num_lights_lit = map[2][0].is_lit() + map[3][7].is_lit();
    top->result = (num_lights_lit << 8) | (max_height_reached & 0xff);
#else
#ifdef TIMING
    get_timestamp(bt0);
#endif
    top->result = program_execute<false>(&top->prg, map, NULL);
#ifdef TIMING
    get_timestamp(bt1);
    timestamp_diff(bt0, bt1);
    timestamp_add(executing, bt0);
#endif
    num_lights_lit = top->result >> 8;
#endif /* JIT */
    if (top->result <= prev_result) continue;
    if (num_lights_lit == 2)
    {
      program_print(&top->prg, stream);
    }
    prev_result = top->result;
    struct stack_item *next = new stack_item();
    next->next = top;
    next->mutate_cnt = 0;
    top = next;
  }
  delete top;
  fclose(stream);
  return 0;
}
