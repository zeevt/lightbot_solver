#include <stdint.h>

/*
You must compile this file with -fomit-frame-pointer
because it uses %rbp as a general purpose register.
The generated at run-time driver that calls these functions does comply with
ABI and can be called from code that uses %rbp in the normal manner.
*/

/*
RDI = y
RSI = x
RDX = dir
RCX = map
R8  = nexy.y
R9  = nexy.x
*/

void do_right(int y, int x, int dir, void *map)
{
  asm(
    "incb %dl\n"
    "andb $3, %dl\n"
  );
}

void do_left(int y, int x, int dir, void *map)
{
  asm(
    "addb $3, %dl\n"
    "andb $3, %dl\n"
  );
}

void do_light(int y, int x, int dir, void *map)
{
  /*
  unsigned char *m = (unsigned char *)map;
  unsigned char *c = &m[(y << 3) + x];
  *c = (((*c >> 1) ^ *c) & 64) | (*c & ~64);
  */
  asm(
    "leaq  (%rsi,%rdi,8), %r8\n"
    "movb  (%rcx,%r8), %al\n"
    "movb  %al, %bl\n"
    "shr   %al\n"
    "xorb  %al, %bl\n"
    "andb  $0xbf, %bl\n"
    "andb  $0x40, %al\n"
    "orb   %bl, %al\n"
    "movb  %al, (%rcx,%r8)\n"
  );
}

static void __attribute__((noinline)) do_before_step(int y, int x, int dir, void *map)
{
  asm(
    "movq %rdi, %r8\n"
    "movq %rsi, %r9\n"
    "cmpb $0,   %dl\n"
    "je   case_n\n"
    "cmpb $1,   %dl\n"
    "je   case_e\n"
    "cmpb $2,   %dl\n"
    "je   case_s\n"
    "case_w:\n"
    "decb %r9b\n"
    "movb %r9b, %al\n"
    "sarb $7,   %al\n"
    "notb %al\n"
    "andb %al,  %r9b\n"
    "jmp  step_end\n"
    "case_s:\n"
    "decb %r8b\n"
    "movb %r8b, %al\n"
    "sarb $7,   %al\n"
    "notb %al\n"
    "andb %al,  %r8b\n"
    "jmp  step_end\n"
    "case_e:\n"
    "incb %r9b\n"
    "subb $7,   %r9b\n"/* max_x_coord */
    "movb %r9b, %al\n"
    "sarb $7,   %al\n" /* CHAR_BITS - 1 */
    "andb %al,  %r9b\n"
    "addb $7,   %r9b\n"/* max_x_coord */
    "jmp  step_end\n"
    "case_n:\n"
    "incb %r8b\n"
    "subb $4,   %r8b\n"/* max_y_coord */
    "movb %r8b, %al\n"
    "sarb $7,   %al\n" /* CHAR_BITS - 1 */
    "andb %al,  %r8b\n"
    "addb $4,   %r8b\n"/* max_y_coord */
    "step_end:\n"
    "leaq (%rsi,%rdi,8), %rax\n"
    "leaq (%r9, %r8, 8), %rbx\n"
    "movb (%rcx,%rax,1), %al\n"
    "movb (%rcx,%rbx,1), %bl\n"
    "andb $63,  %al\n"
    "andb $63,  %bl\n"
  );
}

void do_forward(int y, int x, int dir, void *map)
{
  do_before_step(y, x, dir, map);
  asm(
    "cmp  %al,  %bl\n"
    "jne  forward_end\n"
    "mov  %r8,  %rdi\n"
    "mov  %r9,  %rsi\n"
    "forward_end:\n"
  );
}

void do_jump(int y, int x, int dir, void *map)
{
  do_before_step(y, x, dir, map);
  asm(
    "cmp  %al,  %bl\n"
    "jb   perform_jump\n"
    "inc  %al\n"
    "cmp  %al,   %bl\n"
    "jne  jump_end\n"
    "cmp  %bl,  %bpl\n"
    "jae  perform_jump\n"
    "mov  %bl,  %bpl\n"
    "perform_jump:\n"
    "mov  %r8,  %rdi\n"
    "mov  %r9,  %rsi\n"
    "jump_end:\n"
  );
}
