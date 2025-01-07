#include <SDL2/SDL.h>

#include <iostream>

#include "src/chip8.h"
using namespace std;

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    cerr << "Usage: " << argv[0] << " <ROM file>" << std::endl;
    return 1;
  }

  Chip8 chip8;
  chip8.loadROM(argv[1]);

  bool running = true;
  SDL_Event e;

  while (running)
  {
    while (SDL_PollEvent(&e) != 0)
    {
      if (e.type == SDL_QUIT)
      {
        running = false;
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
    chip8.draw();

    SDL_Delay(2);
  }

  return 0;
}
