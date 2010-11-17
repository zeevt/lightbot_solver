#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include "lightbot_solver.h"
#include "jit.h"

/*
RDI = y
RSI = x
RDX = dir
RCX = map
R8  = nexy.y
R9  = nexy.x
*/

/*
TODO:
benchmark aligning func entry to 16 byte
benchmark tail calling last func in function
*/


#define handle_error(msg) \
  do { perror(msg); exit(EXIT_FAILURE); } while (0)

// 5 * 12 + 9 = 69 --> 80
// 5 * 8  + 1 = 41 --> 48
#define F1_AMD64_START      80
#define F2_AMD64_START      80 + 48
#define GENERATED_CODE_SIZE 176

JITter::JITter()
{
  this->generated_code = NULL;
  int pagesize = sysconf(_SC_PAGE_SIZE);
  if (pagesize == -1)
    handle_error("sysconf");
  
  if (posix_memalign(&this->generated_code, pagesize, GENERATED_CODE_SIZE))
    handle_error("posix_memalign");
  
  if (mprotect(this->generated_code, GENERATED_CODE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC))
    handle_error("mprotect");
  
  uint8_t *p = (uint8_t *)this->generated_code;
  this->adjusted_target[right] = (int64_t)&do_right - 5;
  this->adjusted_target[left] = (int64_t)&do_left - 5;
  this->adjusted_target[f1] = (int64_t)&p[F1_AMD64_START] - 5;
  this->adjusted_target[f2] = (int64_t)&p[F2_AMD64_START] - 5;
  this->adjusted_target[light] = (int64_t)&do_light - 5;
  this->adjusted_target[forward] = (int64_t)&do_forward - 5;
  this->adjusted_target[jump] = (int64_t)&do_jump - 5;
}

JITter::~JITter()
{
  if (this->generated_code)
    free(this->generated_code);
}

void JITter::generate_code(const struct program_t *prg)
{
  uint8_t *generated_codep = (uint8_t *)this->generated_code;
  uint8_t *p = generated_codep;
  
#define WRITE_CALL(cmd) \
  do { \
    int curr_cmd = prg->cmds[cmd]; \
    if (curr_cmd == nop) continue; \
    int64_t next_rip = (int64_t)p; \
    int64_t target = adjusted_target[curr_cmd]; \
    int64_t diff = target - next_rip; \
    *(p++) = '\xe8';/* call */ \
    *(int32_t*)p = (int32_t)(diff); \
    p += 4; \
  } while(0)
  
  // push %rbp ; push %rbx ; xor %ebp, %ebp
  *(uint32_t*)p = 0xed315355;
  p += 4;
  WRITE_CALL(0);
  WRITE_CALL(1);
  WRITE_CALL(2);
  WRITE_CALL(3);
  WRITE_CALL(4);
  WRITE_CALL(5);
  WRITE_CALL(6);
  WRITE_CALL(7);
  WRITE_CALL(8);
  WRITE_CALL(9);
  WRITE_CALL(10);
  WRITE_CALL(11);
  // mov %ebp, %eax ; pop %rbx ; pop %rbp
  *(uint32_t*)p = 0x5d5be889;
  p += 4;
  *p = '\xc3';    // retq
  p = &generated_codep[F1_AMD64_START];
  WRITE_CALL(12);
  WRITE_CALL(13);
  WRITE_CALL(14);
  WRITE_CALL(15);
  WRITE_CALL(16);
  WRITE_CALL(17);
  WRITE_CALL(18);
  WRITE_CALL(19);
  *p = '\xc3';    // retq
  p = &generated_codep[F2_AMD64_START];
  WRITE_CALL(20);
  WRITE_CALL(21);
  WRITE_CALL(22);
  WRITE_CALL(23);
  WRITE_CALL(24);
  WRITE_CALL(25);
  WRITE_CALL(26);
  WRITE_CALL(27);
  *p = '\xc3';    // retq
#ifdef DEBUG
  for (int i = 0; i < GENERATED_CODE_SIZE; i++)
    printf("%2x ", generated_codep[i]);
  printf("\n");
#endif
}

int JITter::run_program(int y, int x, int dir, void *map)
{
  typedef int (*do_program_t)(int y, int x, int dir, void *map);
  do_program_t do_program = (do_program_t)this->generated_code;
  int result = do_program(y, x, dir, map);
  return result;
}

#ifdef TEST
#include <string.h>

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
  
  JITter jitter;
  jitter.generate_code(&prg);
  int result = jitter.run_program(1, 0, 1, &map[0][0]);
  
  int num_lights_lit = map[2][0].is_lit() + map[3][7].is_lit();
  printf("%d %d\n", num_lights_lit, result);
  
  return 0;
}

#endif /* TEST */
