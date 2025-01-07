#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <iterator>
#include <random>
#include <regex>
#include <system_error>

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

const int SCREEN_WIDTH = 64;
const int SCREEN_HEIGHT = 32;
const int SCALE = 10;

const int FONTSET_SIZE = 80;
const int FONTSET_START = 0;

const int MEM_START = 0x200;

class Chip8
{
private:
  uint8_t memory[4096];
  uint32_t video[SCREEN_WIDTH * SCREEN_HEIGHT];
  uint16_t index;

  uint8_t delayTimer;
  uint8_t soundTimer;

  uint8_t registers[16];
  uint16_t programCounter;

  uint16_t stack[16];
  uint8_t stackPointer;

  uint8_t keypad[16];
  uint16_t opCode;

  default_random_engine randGen;
  uniform_int_distribution<uint8_t> randByte;

  uint8_t fontset[FONTSET_SIZE] = {
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

public:
  Chip8() : randGen(std::random_device()())
  {
    programCounter = MEM_START;
    opCode = 0;
    index = 0;
    stackPointer = 0;

    fill(begin(memory), end(memory), 0);
    fill(begin(video), end(video), 0);
    fill(begin(registers), end(registers), 0);
    fill(begin(stack), end(stack), 0);
    fill(begin(keypad), end(keypad), 0);

    for (int i = 0; i < FONTSET_SIZE; i++) memory[FONTSET_START + i] = fontset[i];
    randByte = std::uniform_int_distribution<uint8_t>(0, 255);
  }

  void loadROM(const string &filename)
  {
    ifstream file(filename, ios::binary | ios::ate);
    if (!file.is_open())
      return;

    streampos size = file.tellg();
    char *buffer = new char[size];

    file.seekg(0, ios::beg);
    file.read(buffer, size);
    file.close();

    for (int i = 0; i < size; i++) memory[MEM_START + i] = buffer[i];

    delete[] buffer;
  }

  void execute(uint16_t opCode)
  {
    switch (opCode & 0xF000)  // Check the first nibble
    {
      case 0x0000:
        switch (opCode & 0x000F)
        {
          case 0x0000:  // 00E0 -> Clear the screen
            fill(begin(video), end(video), 0);
            drawFlag = true;
            programCounter += 2;
            break;

          case 0x000E:  // 00EE -> Return from subroutine
            stackPointer--;
            programCounter = stack[stackPointer];
            programCounter += 2;
            break;

          default:
            std::cerr << "Error: Unkown OpCode - " << opCode << std::endl;
            break;
        }
        break;
      case 0x1000:  // 1NNN -> Sets the PC to address NNN;
        programCounter = opCode & 0x0FFF;
        break;
      case 0x2000:  // 2NNN -> calls the subroutine at address NNN;
        stack[stackPointer] = programCounter;
        stackPointer++;
        programCounter = opCode & 0x0FFF;
        break;
      case 0x3000:  // 3XKK -> Skip instruction if Rx = Byte (KK);
        if ((opCode & 0x00FF) == registers[(opCode & 0x0F00) >> 8])
          programCounter += 2;
        programCounter += 2;
        break;

      case 0x4000:  // 4XKK -> Skip instruction if Rx != Byte (KK);
        if ((opCode & 0x00FF) != registers[(opCode & 0x0F00) >> 8])
          programCounter += 2;
        programCounter += 2;
        break;

      case 0x5000:  // 5XY0 -> Skip instruction if Rx == Ry;
        if (registers[(opCode & 0x00F0) >> 4] == registers[(opCode & 0x0F00) >> 8])
          programCounter += 2;
        programCounter += 2;
        break;

      case 0x6000:  // 6XKK -> Load Byte(KK) in Rx;
        registers[(opCode & 0x0F00) >> 8] = (opCode & 0x00FF);
        programCounter += 2;
        break;

      case 0x7000:  // 7XKK -> ADD Byte(KK) in Rx;
        registers[(opCode & 0x0F00) >> 8] += (opCode & 0x00FF);
        programCounter += 2;
        break;

      case 0x8000:  // 8___
        switch (opCode & 0x000F)
        {
          case 0x0000:  // 8XY0 -> Load Ry into Rx
            registers[(opCode & 0x0F00) >> 8] = registers[(opCode & 0x00F0) >> 4];
            programCounter += 2;
            break;

          case 0x0001:  // 8XY1 -> Rx = Rx OR Ry;
            registers[(opCode & 0x0F00) >> 8] |= registers[(opCode & 0x00F0) >> 4];
            programCounter += 2;
            break;

          case 0x0002:  // 8XY2 -> Rx = Rx AND Ry;
            registers[(opCode & 0x0F00) >> 8] &= registers[(opCode & 0x00F0) >> 4];
            programCounter += 2;
            break;

          case 0x0003:  // 8XY3 -> Rx = Rx XOR Ry;
            registers[(opCode & 0x0F00) >> 8] ^= registers[(opCode & 0x00F0) >> 4];
            programCounter += 2;
            break;

          case 0x0004:  // 8XY4 -> Rx = Rx + Ry and Rf = Carry;
            registers[0xF] = (registers[(opCode & 0x0F00) >> 8] > (0xFF - registers[(opCode & 0x00F0) >> 4])) ? 1 : 0;
            registers[(opCode & 0x0F00) >> 8] += registers[(opCode & 0x00F0) >> 4];
            programCounter += 2;
            break;

          case 0x0005:  // 8XY5 -> Rx = Rx - Ry and Rf = Not Borrow;
            registers[0xF] = (registers[(opCode & 0x0F00) >> 8] > registers[(opCode & 0x00F0) >> 4]) ? 1 : 0;  // Rx > Ry -> Rf = 1
            registers[(opCode & 0x0F00) >> 8] -= registers[(opCode & 0x00F0) >> 4];
            programCounter += 2;
            break;

          case 0x0006:  // 8XY6 -> Vx SHR 1
            registers[0xF] = registers[(opCode & 0x0F00) >> 8] & 0x1;
            registers[(opCode & 0x0F00) >> 8] >>= 1;
            programCounter += 2;
            break;

          case 0x0007:  // 8XY7 -> Rx = Ry - Rx and Rf = Not Borrow;
            registers[0xF] = (registers[(opCode & 0x00F0) >> 4] > registers[(opCode & 0x0F00) >> 8]) ? 1 : 0;  // Ry > Rx -> Rf = 1
            registers[(opCode & 0x0F00) >> 8] = registers[(opCode & 0x00F0) >> 4] - registers[(opCode & 0x0F00) >> 8];
            programCounter += 2;
            break;

          case 0x000E:  // 8XYE -> Rx = Rx SHL 1, Rf = MSB before shift
            registers[0xF] = registers[(opCode & 0x0F00) >> 8] >> 7;
            registers[(opCode & 0x0F00) >> 8] <<= 1;
            programCounter += 2;
            break;

          default:
            std::cerr << "Error: Unkown OpCode - " << opCode << std::endl;
            break;
        }
        break;

      case 0x9000:  // 9XY0 -> Skip if Rx != Ry;
        if (registers[(opCode & 0x00F0) >> 4] != registers[(opCode & 0x0F00) >> 8])
          programCounter += 2;
        programCounter += 2;
        break;
      case 0xA000:  // ANNN -> Set Ri = NNN;
        index = opCode & 0x0FFF;
        programCounter += 2;
        break;
      case 0xB000:  // BNNN -> Skip to address R0 + NNN
        programCounter = (opCode & 0x0FFF) + registers[0];
        break;
      case 0xC000:  // CXKK -> Set Vx random and KK;
        registers[(opCode & 0x0F00) >> 8] = randByte(randGen) & (opCode & 0x00FF);
        programCounter += 2;
        break;
      case 0xD000:  // DXYN -> Draw sprite at (Rx, Ry), width 8, height N
      {
        uint8_t x = registers[(opCode & 0x0F00) >> 8];
        uint8_t y = registers[(opCode & 0x00F0) >> 4];
        uint8_t height = opCode & 0x000F;
        registers[0xF] = 0;

        for (int yline = 0; yline < height; yline++)
        {
          uint8_t pixel = memory[index + yline];
          for (int xline = 0; xline < 8; xline++)
          {
            if ((pixel & (0x80 >> xline)) != 0)
            {
              if (video[(x + xline + ((y + yline) * 64))] == 1)
                registers[0xF] = 1;
              video[(x + xline + ((y + yline) * 64))] ^= 1;
            }
          }
        }

        drawFlag = true;
        programCounter += 2;
        break;
      }
      break;
      case 0xE000:  // E___
        switch (opCode & 0x00FF)
        {
          case 0x009E:  // EX9E -> Skip next instruction if key in Rx is pressed
            if (keypad[registers[(opCode & 0x0F00) >> 8]] != 0)
              programCounter += 2;
            programCounter += 2;
            break;

          case 0x00A1:  // EXA1 -> Skip next instruction if key in Rx is not pressed
            if (keypad[registers[(opCode & 0x0F00) >> 8]] == 0)
              programCounter += 2;
            programCounter += 2;
            break;

          default:
            std::cerr << "Error: Unkown OpCode - " << opCode << std::endl;
            break;
        }
        break;

      case 0xF000:
        switch (opCode & 0x00FF)
        {
          case 0x0007:  // FX07 -> Set Rx = delay timer
            registers[(opCode & 0x0F00) >> 8] = delayTimer;
            programCounter += 2;
            break;

          case 0x000A:  // FX0A -> Wait for key press, store in Rx

          {
            bool key_pressed = false;
            for (int i = 0; i < 16; ++i)
            {
              if (keypad[i] != 0)
              {
                registers[(opCode & 0x0F00) >> 8] = i;
                key_pressed = true;
              }
            }

            if (!key_pressed)
              return;
            programCounter += 2;
            break;
          }

          case 0x0015:  // FX15 -> Set delay timer = Rx
            delayTimer = registers[(opCode & 0x0F00) >> 8];
            programCounter += 2;
            break;

          case 0x0018:  // FX18 -> Set sound timer = Rx
            soundTimer = registers[(opCode & 0x0F00) >> 8];
            programCounter += 2;
            break;

          case 0x001E:  // FX1E -> Set I = I + Rx
            registers[0xF] = (index + registers[(opCode & 0x0F00) >> 8] > 0xFFF) ? 1 : 0;
            index += registers[(opCode & 0x0F00) >> 8];
            programCounter += 2;
            break;

          case 0x0029:  // FX29 -> Set I = location of sprite for digit Rx
            index = registers[(opCode & 0x0F00) >> 8] * 0x5;
            programCounter += 2;
            break;

          case 0x0033:  // FX33 ->  Store BCD representation of Rx in memory locations I, I+1, I+2
            memory[index] = registers[(opCode & 0x0F00) >> 8] / 100;
            memory[index + 1] = (registers[(opCode & 0x0F00) >> 8] / 10) % 10;
            memory[index + 2] = registers[(opCode & 0x0F00) >> 8] % 10;
            programCounter += 2;
            break;

          case 0x0055:  // FX55 -> Store registers R0 to Rx in memory starting at location I
            for (int i = 0; i <= ((opCode & 0x0F00) >> 8); i++) memory[index + i] = registers[i];
            index += ((opCode & 0x0F00) >> 8) + 1;
            programCounter += 2;
            break;

          case 0x0065:  // FX65 -> Read registers R0 to Rx from memory starting at location I
            for (int i = 0; i <= ((opCode & 0x0F00) >> 8); i++) registers[i] = memory[index + i];
            index += ((opCode & 0x0F00) >> 8) + 1;
            programCounter += 2;
            break;

          default:
            std::cerr << "Error: Unkown OpCode - " << opCode << std::endl;
            break;
        }
        break;

      default:
        std::cerr << "Error: Unkown OpCode - " << opCode << std::endl;
        break;
    }
  }
  void emulateCycle()
  {
    opCode = memory[programCounter] << 8 | memory[programCounter + 1];
    execute(opCode);

    if (delayTimer > 0)
      delayTimer--;

    if (soundTimer > 0)
    {
      if (soundTimer == 1)
        cout << "BEEP!" << endl;
      soundTimer--;
    }
  }

  void draw(SDL_Renderer *renderer)
  {
    if (!drawFlag)
      return;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);  // set draw color to Black;
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    for (int x = 0; x < SCREEN_WIDTH; x++)
    {
      for (int y = 0; y < SCREEN_HEIGHT; y++)
        if (video[(y * SCREEN_WIDTH) + x])
        {
          SDL_Rect box = {x * SCALE, y * SCALE, SCALE, SCALE};
          SDL_RenderFillRect(renderer, &box);
        }
    }

    SDL_RenderPresent(renderer);
    drawFlag = false;
  }

  void setKeys(const uint8_t *keys)
  {
    for (int i = 0; i < 16; ++i) keypad[i] = keys[i];
  }
};

int main(int argc, char **argv)
{
  if (argc < 1)
  {
    cerr << "USAGE: " << argv[0] << " <ROM File>";
    return 1;
  }

  SDL_Init(SDL_INIT_VIDEO);

  SDL_Window *window =
      SDL_CreateWindow("CHIP-8 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH * SCALE, SCREEN_HEIGHT * SCALE, 0);
  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  Chip8 chip8;
  chip8.loadROM(argv[1]);

  bool quit = false;
  SDL_Event e;

  while (!quit)
  {
    while (SDL_PollEvent(&e) != 0)
    {
      if (e.type == SDL_QUIT)
      {
        quit = true;
      }
      else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
      {
        uint8_t keys[16] = {0};
        const uint8_t *state = SDL_GetKeyboardState(NULL);

        keys[0x1] = state[SDL_SCANCODE_1];
        keys[0x2] = state[SDL_SCANCODE_2];
        keys[0x3] = state[SDL_SCANCODE_3];
        keys[0xC] = state[SDL_SCANCODE_4];

        keys[0x4] = state[SDL_SCANCODE_Q];
        keys[0x5] = state[SDL_SCANCODE_W];
        keys[0x6] = state[SDL_SCANCODE_E];
        keys[0xD] = state[SDL_SCANCODE_R];

        keys[0x7] = state[SDL_SCANCODE_A];
        keys[0x8] = state[SDL_SCANCODE_S];
        keys[0x9] = state[SDL_SCANCODE_D];
        keys[0xE] = state[SDL_SCANCODE_F];

        keys[0xA] = state[SDL_SCANCODE_Z];
        keys[0x0] = state[SDL_SCANCODE_X];
        keys[0xB] = state[SDL_SCANCODE_C];
        keys[0xF] = state[SDL_SCANCODE_V];

        chip8.setKeys(keys);
      }
    }

    chip8.emulateCycle();
    chip8.draw(renderer);

    SDL_Delay(2);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
