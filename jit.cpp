#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include "lightbot_solver.h"

/*
RDI = y
RSI = x
RDX = dir
RCX = map
*/

/*
TODO:
add max height reached calculation (place in ebp?)
benchmark aligning func entry to 16 byte
benchmark tail calling last func in function
*/

void do_right(void)
{
  asm(
    "inc %rdx\n"
    "and $3, %rdx\n"
  );
}

void do_left(void)
{
  asm(
    "add $3, %rdx\n"
    "and $3, %rdx\n"
  );
}

void do_light(void)
{
  asm(
    "mov %rdi, %r8\n"
    "shl $3, %r8\n"
    "add %rsi, %r8\n"
    "movb (%rcx,%r8,1), %al\n"
    "testb $128, %al\n"
    "jz light_end\n"
    "movb %al, %bl\n"
    "andb $191, %al\n"
    "notb %bl\n"
    "andb $64, %bl\n"
    "orb  %bl, %al\n"
    "movb %al, (%rcx,%r8,1)\n"
    "light_end:\n"
  );
}

void __attribute__((noinline)) do_before_step(void)
{
  asm(
  "mov  %rdi, %r8\n"
  "mov  %rsi, %r9\n"
  "cmp  $0,   %rdx\n"
  "je   case_n\n"
  "cmp  $1,   %rdx\n"
  "je   case_e\n"
  "cmp  $2,   %rdx\n"
  "je   case_s\n"
  "case_w:\n"
  "cmp  $0,   %r9\n"
  "jbe  step_end\n"
  "dec  %r9\n"
  "jmp  step_end\n"
  "case_s:\n"
  "cmp  $0,   %r8\n"
  "jbe  step_end\n"
  "dec  %r8\n"
  "jmp  step_end\n"
  "case_e:\n"
  "cmp  $6,   %r9\n"
  "ja   step_end\n"
  "inc  %r9\n"
  "jmp  step_end\n"
  "case_n:\n"
  "cmp  $3,   %r8\n"
  "ja   step_end\n"
  "inc  %r8\n"
  "step_end:\n"
  "mov  %rdi, %rax\n"
  "shl  $3,   %rax\n"
  "add  %rsi, %rax\n"
  "movb (%rcx,%rax,1), %al\n"
  "andb $63,  %al\n"
  "mov  %r8,  %rbx\n"
  "shl  $3,   %rbx\n"
  "add  %r9,  %rbx\n"
  "movb (%rcx,%rbx,1), %bl\n"
  "andb $63,  %bl\n"
  );
}

void do_forward(void)
{
  do_before_step();
  asm(
    "cmp  %al,  %bl\n"
    "jne  forward_end\n"
    "mov  %r8,  %rdi\n"
    "mov  %r9,  %rsi\n"
    "forward_end:\n"
  );
}

void do_jump(void)
{
  do_before_step();
  asm(
    "cmp  %al,  %bl\n"
    "jb   perform_jump\n"
    "inc  %al\n"
    "cmp  %al,   %bl\n"
    "jne  jump_end\n"
    "perform_jump:\n"
    "mov  %r8,  %rdi\n"
    "mov  %r9,  %rsi\n"
    "jump_end:\n"
  );
}

#define handle_error(msg) \
  do { perror(msg); exit(EXIT_FAILURE); } while (0)

// 5 * 12 + 5 = 65 --> 80
// 5 * 8 + 1 = 41 --> 48
#define F1_AMD64_START      80
#define F2_AMD64_START      80 + 48
#define GENERATED_CODE_SIZE 176

class JITter
{
public:
  JITter();
  ~JITter();
  void generate_code(const struct program_t *prg);
  void run_program(int y, int x, int dir, void *map);
private:
  void *generated_code;
  typedef void (*func_t)(void);
  func_t funcs[7];
};

JITter::JITter()
{
  this->generated_code = NULL;
  int pagesize = sysconf(_SC_PAGE_SIZE);
  if (pagesize == -1)
    handle_error("sysconf");
  
  if (posix_memalign(&this->generated_code, pagesize, GENERATED_CODE_SIZE))
    handle_error("posix_memalign");
  
  uint8_t *p = (uint8_t *)this->generated_code;
  this->funcs[right] = &do_right;
  this->funcs[left] = &do_left;
  this->funcs[f1] = (void (*)())&p[F1_AMD64_START];
  this->funcs[f2] = (void (*)())&p[F2_AMD64_START];
  this->funcs[light] = &do_light;
  this->funcs[forward] = &do_forward;
  this->funcs[jump] = &do_jump;
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
  *(p++) = '\x55';// push   %rbp
  *(p++) = '\x53';// push   %rbx
  for (int cmd = 0; cmd < CMDS_IN_PRG; cmd++)
  {
    int curr_cmd = prg->cmds[cmd];
    if (curr_cmd != nop)
    {
      int64_t next_rip = (int64_t)p + 5;
      int64_t target = (int64_t)funcs[curr_cmd];
      int64_t diff = target - next_rip;
      *(p++) = '\xe8';// retq
      *(int32_t*)p = (int32_t)(diff);
      p += 4;
    }
    if (cmd == F1_START - 1)
    {
      *(p++) = '\x5b';// pop   %rbx
      *(p++) = '\x5d';// pop   %rbp
      *p = '\xc3';    // retq
      p = &generated_codep[F1_AMD64_START];
    }
    else if (cmd == F2_START - 1)
    {
      *p = '\xc3';    // retq
      p = &generated_codep[F2_AMD64_START];
    }
    else if (cmd == CMDS_IN_PRG - 1)
    {
      *p = '\xc3';    // retq
    }
  }
#ifdef DEBUG
  for (int i = 0; i < GENERATED_CODE_SIZE; i++)
    printf("%2x ", generated_codep[i]);
  printf("\n");
#endif
}

void JITter::run_program(int y, int x, int dir, void *map)
{
  if (mprotect(this->generated_code, GENERATED_CODE_SIZE, PROT_READ | PROT_EXEC))
    handle_error("mprotect");
  typedef void(*do_program_t)(int y, int x, int dir, void *map);
  do_program_t do_program = (do_program_t)this->generated_code;
  do_program(y, x, dir, map);
  if (mprotect(this->generated_code, GENERATED_CODE_SIZE, PROT_READ | PROT_WRITE))
    handle_error("mprotect");
}

#ifdef TEST

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
  jitter.run_program(1, 0, 1, &map[0][0]);
  
  int num_lights_lit = map[2][0].is_lit() + map[3][7].is_lit();
  printf("%d\n", num_lights_lit);
  
  return 0;
}

#endif /* TEST */
