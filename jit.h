#ifndef jit_h_
#define jit_h_

extern "C" {
void do_right(int y, int x, int dir, void *map);
void do_left(int y, int x, int dir, void *map);
void do_light(int y, int x, int dir, void *map);
void do_forward(int y, int x, int dir, void *map);
void do_jump(int y, int x, int dir, void *map);
}

class JITter
{
public:
  JITter();
  ~JITter();
  void generate_code(const struct program_t *prg);
  int run_program(int y, int x, int dir, void *map);
private:
  void *generated_code;
  typedef void (*func_t)(int y, int x, int dir, void *map);
  func_t funcs[7];
};

#endif /* jit_h_ */
