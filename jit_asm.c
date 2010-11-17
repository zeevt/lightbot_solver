
/*
You must compile this file with -fomit-frame-pointer
because it uses %rbp as a general purpose register.
The generated at run-time driver that calls these functions does comply with
ABI and can be called from code that uses %rbp in the normal manner.
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
  asm(
    "leaq  (%rsi,%rdi,8), %r8\n"
    "movb  (%rcx,%r8), %al\n"
    "testb $128, %al\n"
    "jz    light_end\n"
    "movb  %al,  %bl\n"
    "andb  $191, %al\n"
    "notb  %bl\n"
    "andb  $64,  %bl\n"
    "orb   %bl,  %al\n"
    "movb  %al,  (%rcx,%r8)\n"
    "light_end:\n"
  );
}

static void __attribute__((noinline)) do_before_step(int y, int x, int dir, void *map)
{
  asm(
  "leaq (%rsi,%rdi,8), %rax\n"
  "movb (%rcx,%rax,1), %al\n"
  "andb $63,  %al\n"
  "movq %rdi, %r8\n"
  "movq %rsi, %r9\n"
  "cmpb $0,   %dl\n"
  "je   case_n\n"
  "cmpb $1,   %dl\n"
  "je   case_e\n"
  "cmpb $2,   %dl\n"
  "je   case_s\n"
  "case_w:\n"
  "cmpb $0,   %sil\n"
  "jbe  step_end\n"
  "decb %r9b\n"
  "jmp  step_end\n"
  "case_s:\n"
  "cmpb $0,   %dil\n"
  "jbe  step_end\n"
  "decb %r8b\n"
  "jmp  step_end\n"
  "case_e:\n"
  "cmpb $6,   %sil\n"
  "ja   step_end\n"
  "incb %r9b\n"
  "jmp  step_end\n"
  "case_n:\n"
  "cmpb $3,   %dil\n"
  "ja   step_end\n"
  "incb %r8b\n"
  "step_end:\n"
  "leaq (%r9,%r8,8), %rbx\n"
  "movb (%rcx,%rbx,1), %bl\n"
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
