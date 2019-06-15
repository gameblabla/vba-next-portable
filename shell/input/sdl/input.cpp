#include <SDL/SDL.h>
#include <stdint.h>
#include <stdio.h>

#include "menu.h"
#include "config.h"

int32_t update_input(void)
{
	SDL_Event event;
	int32_t button = 0;
	uint8_t* keys;
	
	keys = SDL_GetKeyState(NULL);
	
	SDL_PollEvent(&event);
	
	if ((keys[SDLK_RETURN] == SDL_PRESSED && keys[SDLK_ESCAPE] == SDL_PRESSED) || keys[SDLK_END] == SDL_PRESSED) emulator_state = 1;

	// UP
	if (keys[option.config_buttons[0] ] == SDL_PRESSED)
	{
		button |= 64;
	}
	
	// DOWN
	if (keys[option.config_buttons[1] ] == SDL_PRESSED)
	{
		button |= 16;
	}
	
	// LEFT
	if (keys[option.config_buttons[2] ] == SDL_PRESSED)
	{
		button |= 128;
	}
	
	// Right
	if (keys[option.config_buttons[3] ] == SDL_PRESSED)
	{
		button |= 32;
	}
	
	// A
	if (keys[option.config_buttons[4] ] == SDL_PRESSED)
	{
		button |= 1;
	}
	
	// B
	if (keys[option.config_buttons[5] ] == SDL_PRESSED)
	{
		button |= 2;
	}
	
	// L
	if (keys[option.config_buttons[6] ] == SDL_PRESSED)
	{
		button |= 512;
	}
	
	// R
	if (keys[option.config_buttons[7] ] == SDL_PRESSED)
	{
		button |= 256;
	}

	// Start
	if (keys[option.config_buttons[8] ] == SDL_PRESSED)
	{
		button |= 8;
	}

	// Select
	if (keys[option.config_buttons[9] ] == SDL_PRESSED)
	{
		button |= 4;
	}
	
	
	return button;
}

