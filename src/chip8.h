#ifndef CHIP8_H
#define CHIP8_H

#include <SDL2/SDL.h>

#include <array>
#include <cstdint>
#include <random>
#include <string>

using namespace std;

// Specification:
// 4 KB RAM
// 64 x 32 pixels monochrome display
// 16 bit index register - I
// 8 bit delay timer
// 8 bit sound timer
// 16 8-bit (1 byte) general-purpose registers
// 16 bit stack
// 16 key - keypad

class Chip8
{
public:
  Chip8();
  ~Chip8();
  void loadROM(const string &filename);
  void emulateCycle();
  void draw();
  void setKeys(const uint8_t *keys);

private:
  void execute(uint16_t opCode);

  static const int SCREEN_WIDTH = 64;
  static const int SCREEN_HEIGHT = 32;
  static const int SCALE = 10;
  static const int FONTSET_SIZE = 80;
  static const int FONTSET_START = 0;
  static const int MEM_START = 0x200;

  array<uint8_t, 4096> memory;
  array<uint32_t, SCREEN_WIDTH * SCREEN_HEIGHT> video;

  uint16_t index;
  uint8_t delayTimer;
  uint8_t soundTimer;
  array<uint8_t, 16> registers;
  uint16_t programCounter;
  array<uint16_t, 16> stack;
  uint8_t stackPointer;

  array<uint8_t, 16> keypad;

  uint16_t opCode;

  default_random_engine randGen;
  uniform_int_distribution<uint8_t> randByte;

  SDL_Window *window;
  SDL_Renderer *renderer;

  const array<uint8_t, FONTSET_SIZE> fontset = {
      0xF0, 0x90, 0x90, 0x90, 0xF0,  // 0
      0x20, 0x60, 0x20, 0x20, 0x70,  // 1
      0xF0, 0x10, 0xF0, 0x80, 0xF0,  // 2
      0xF0, 0x10, 0xF0, 0x10, 0xF0,  // 3
      0x90, 0x90, 0xF0, 0x10, 0x10,  // 4
      0xF0, 0x80, 0xF0, 0x10, 0xF0,  // 5
      0xF0, 0x80, 0xF0, 0x90, 0xF0,  // 6
      0xF0, 0x10, 0x20, 0x40, 0x40,  // 7
      0xF0, 0x90, 0xF0, 0x90, 0xF0,  // 8
      0xF0, 0x90, 0xF0, 0x10, 0xF0,  // 9
      0xF0, 0x90, 0xF0, 0x90, 0x90,  // A
      0xE0, 0x90, 0xE0, 0x90, 0xE0,  // B
      0xF0, 0x80, 0x80, 0x80, 0xF0,  // C
      0xE0, 0x90, 0x90, 0x90, 0xE0,  // D
      0xF0, 0x80, 0xF0, 0x80, 0xF0,  // E
      0xF0, 0x80, 0xF0, 0x80, 0x80   // F
  };

  bool drawFlag;
};

#endif  // !CHIP8
