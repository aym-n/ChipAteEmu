#include "chip8.h"

#include <SDL2/SDL.h>

#include <fstream>
#include <iostream>

Chip8::Chip8() : randGen(std::random_device()())
{
  programCounter = MEM_START;
  opCode = 0;
  index = 0;
  stackPointer = 0;

  memory.fill(0);
  video.fill(0);
  registers.fill(0);
  stack.fill(0);
  keypad.fill(0);

  for (int i = 0; i < FONTSET_SIZE; i++) memory[FONTSET_START + i] = fontset[i];

  randByte = std::uniform_int_distribution<uint8_t>(0, 255);

  if (SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    std::cerr << "couldn't initalize SDL - SDL_Error: " << SDL_GetError() << std::endl;
    exit(1);
  }

  window =
      SDL_CreateWindow("ChipAteEmu-lator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH * SCALE, SCREEN_HEIGHT * SCALE, 0);
  if (window == nullptr)
  {
    std::cerr << "Couldn't create window - SDL_Error: " << SDL_GetError() << std::endl;
    SDL_Quit();
    exit(1);
  }

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (renderer == nullptr)
  {
    std::cerr << "Coundn't create renderer - SDL_Error: " << SDL_GetError() << std::endl;
    SDL_DestroyWindow(window);
    SDL_Quit();
    exit(1);
  }

  drawFlag = false;
}

Chip8::~Chip8()
{
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

void Chip8::loadROM(const string &filename)
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

void Chip8::emulateCycle()
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

void Chip8::draw()
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

void Chip8::setKeys(const uint8_t *keys)
{
  for (int i = 0; i < 16; ++i) keypad[i] = keys[i];
}

void Chip8::execute(uint16_t opCode)
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
