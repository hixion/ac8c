#include "chip.h"
#include <random>

static const word OP_OFFSET = 2;
static const byte SCREEN_WIDTH = 64;
static const byte SCREEN_HEIGHT = 32;

// Sprites of Chip 8
std::vector<byte> FONT_SET = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, 0x20, 0x60, 0x20, 0x20, 0x70, 0xF0, 0x10,
    0xF0, 0x80, 0xF0, 0xF0, 0x10, 0xF0, 0x10, 0xF0, 0x90, 0x90, 0xF0, 0x10,
    0x10, 0xF0, 0x80, 0xF0, 0x10, 0xF0, 0xF0, 0x80, 0xF0, 0x90, 0xF0, 0xF0,
    0x10, 0x20, 0x40, 0x40, 0xF0, 0x90, 0xF0, 0x90, 0xF0, 0xF0, 0x90, 0xF0,
    0x10, 0xF0, 0xF0, 0x90, 0xF0, 0x90, 0x90, 0xE0, 0x90, 0xE0, 0x90, 0xE0,
    0xF0, 0x80, 0x80, 0x80, 0xF0, 0xE0, 0x90, 0x90, 0x90, 0xE0, 0xF0, 0x80,
    0xF0, 0x80, 0xF0, 0xF0, 0x80, 0xF0, 0x80, 0x80,
};

Chip::Chip()
    : rt(0), rd(0), sp(0), i(0), pc(0x0200), stack(std::vector<word>(16, 0)),
      memory(std::vector<byte>(0x1000, 0)), registers(std::vector<byte>(16, 0)),
      keyboard(std::vector<bool>(16, 0)),
      screen(std::vector<std::vector<byte>>(64, std::vector<byte>(32, 0))) {
  for (unsigned i = 0; i < FONT_SET.size(); i++)
    memory[i] = FONT_SET[i];
}

Chip::~Chip() {}

// Execution functions

word Chip::fetch() {
  word addr = pc;
  return ((memory[addr]) << 8 | memory[addr + 1]);
}

void Chip::execute(word opcode) {

  /*
    ============================
           opcode nibbles
    ============================
      nib1 | nib2 | nib3 | nib4
      xxxx | xxxx | xxxx | xxxx
    ============================
  */
  byte nib4 = ((opcode & 0x000f));
  byte nib3 = ((opcode & 0x00f0) >> 4);
  byte nib2 = ((opcode & 0x0f00) >> 8);
  byte nib1 = ((opcode & 0xf000) >> 12);

  // variables
  byte x = nib2;
  byte y = nib3;
  byte n = nib4;
  byte kk = (opcode & 0x00ff);
  word nnn = (opcode & 0x0fff);

  if (nib1 == 0x0 and nib2 == 0x0 and nib3 == 0xe and nib4 == 0x0)
    op_00e0();
  else if (nib1 == 0x0 and nib2 == 0x0 and nib3 == 0xe and nib4 == 0xe)
    op_00ee();
  else
    pc += OP_OFFSET;
}

void Chip::cycle() {
  loop {
    word opcode = fetch();
    execute(opcode);
  }
}

// ==============
//  Instructions
// ==============

// CLS - clear display
void Chip::op_00e0() {
  for (unsigned i = 0; i < screen.size(); i++)
    fill(screen[i].begin(), screen[i].end(), 0);
  pc += OP_OFFSET;
}

// RET - return from a subroutine
void Chip::op_00ee() {
  sp -= 1;
  pc = stack[sp];
}

// JP - jump to location nnn
void Chip::op_1nnn(word addr) { pc = addr; }

// CALL - call subroutine at nnn
void Chip::op_2nnn(word addr) {
  stack[sp] = pc;
  sp += 1;
  pc = addr;
}

// SE - skip next instruction if vx = kk
void Chip::op_3xkk(byte kk, byte x) {
  if (registers[x] == kk)
    pc += 2 * OP_OFFSET;
  else
    pc += OP_OFFSET;
}

// SNE - skip next instruction if vx != kk
void Chip::op_4xkk(byte kk, byte x) {
  if (registers[x] != kk)
    pc += 2 * OP_OFFSET;
  else
    pc += OP_OFFSET;
}

// SE - skip next instruction if vx = vy
void Chip::op_5xy0(byte x, byte y) {
  if (registers[x] == registers[y])
    pc += 2 * OP_OFFSET;
  else
    pc += OP_OFFSET;
}

// LD - set vx = kk
void Chip::op_6xkk(byte kk, byte x) {
  registers[x] = kk;
  pc += OP_OFFSET;
}

// ADD - set vx = vx + kk
void Chip::op_7xkk(byte kk, byte x) {
  registers[x] = registers[x] + kk;
  pc += OP_OFFSET;
}

// LD - set vx = vy
void Chip::op_8xy0(byte x, byte y) {
  registers[x] = registers[y];
  pc += OP_OFFSET;
}

// OR - set vx = vx or vy
void Chip::op_8xy1(byte x, byte y) {
  registers[x] = registers[x] | registers[y];
  pc += OP_OFFSET;
}

// AND - set vx = vx and x vy
void Chip::op_8xy2(byte x, byte y) {
  registers[x] = registers[x] & registers[y];
  pc += OP_OFFSET;
}

// XOR - set vx = vx xor vy
void Chip::op_8xy3(byte x, byte y) {
  registers[x] = registers[x] ^ registers[y];
  pc += OP_OFFSET;
}

// ADD - set vx = vx + vy, set vf = carry
void Chip::op_8xy4(byte x, byte y) {
  word sum = (word)registers[x] + (word)registers[y];
  registers[0x0f] = (sum > 255);
  registers[x] = (byte)sum;
  pc += OP_OFFSET;
}

// SUB - set vx = vx - vy, set vf = not borrow
void Chip::op_8xy5(byte x, byte y) {
  registers[0x0f] = (registers[x] > registers[y]);
  registers[x] = registers[x] - registers[y];
  pc += OP_OFFSET;
}

// SHR - set vx = vx >> 1
void Chip::op_8xy6(byte x) {
  registers[0x0f] = (registers[x] & 0x01);
  registers[x] = registers[x] >> 1;
  pc += OP_OFFSET;
}

// SUBN - set vx = vx - vy, set vf = not borrow
void Chip::op_8xy7(byte x, byte y) {
  registers[0x0f] = (registers[y] > registers[x]);
  registers[x] = registers[y] - registers[x];
  pc += OP_OFFSET;
}

// SHL - set vx = vx << 1
void Chip::op_8xye(byte x) {
  registers[0x0f] = ((registers[x] & 0x80) >> 7);
  registers[x] = registers[x] << 1;
  pc += OP_OFFSET;
}

// SNE - skip next intruction if vx != vy
void Chip::op_9xy0(byte x, byte y) {
  if (registers[x] != registers[y])
    pc += 2 * OP_OFFSET;
  else
    pc += OP_OFFSET;
}

// Annn - set I = nnn
void Chip::op_annn(word addr) {
  i = addr;
  pc += OP_OFFSET;
}

// Bnnn - set I = nnn
void Chip::op_bnnn(word addr) {
  pc = (word)registers[0x0] + addr;
  pc += OP_OFFSET;
}

// RND - set vx = random byte AND kk;
void Chip::op_cxkk(byte x, byte kk) {
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist(0, 255);
  auto rand = dist(rng);

  registers[x] = rand & kk;
  pc += OP_OFFSET;
}

// DRW -Display n-byte sprite starting at memory location I at (Vx, Vy), set VF
// = collision.
/*
  Steps:
  - read n bytes from memory, starting at address I.
  - for each bit of the sprite check if modifies the pixel.
  - store the bit in the screen.
*/
void Chip::op_dxyn(byte x, byte y, byte n) {
  registers[0x0f] = 0;
  for (unsigned j = this->i; j <= n; j++) {
    // column y
    byte sprite = memory[j];
    byte cy = (registers[y] + j) % SCREEN_HEIGHT;
    for (unsigned bit = 0; bit < 8; bit++) {
      // line x
      byte cx = (registers[x] + j) % SCREEN_WIDTH;
      byte pixel = (sprite >> (7 - bit));
      // check if changes the pixels in the screen
      registers[0x0f] |= pixel & screen[cy][cx];
      screen[cy][cx] = pixel ^ screen[cy][cx];
    }
  }
  pc += OP_OFFSET;
}
