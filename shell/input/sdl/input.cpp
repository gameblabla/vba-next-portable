#include <SDL/SDL.h>
#include <stdint.h>
#include <stdio.h>

#include "menu.h"
#include "config.h"

#ifdef ENABLE_JOYSTICKCODE
#define joy_commit_range 8192
int32_t axis_input[2] = {0, 0};
#endif

int32_t update_input(void)
{
	SDL_Event event;
	int32_t button = 0;
	uint8_t* keys;
	
	keys = SDL_GetKeyState(NULL);
	
	while (SDL_PollEvent(&event))
	{
		switch(event.type)
		{
			case SDL_KEYDOWN:
				switch(event.key.keysym.sym)
				{
#ifdef GKD350_BUG_INPUT
					case SDLK_LSHIFT:
#endif
					case SDLK_END:
					case SDLK_RCTRL:
						emulator_state = 1;
					break;
					default:
					break;
				}
			break;
			case SDL_KEYUP:
				switch(event.key.keysym.sym)
				{
					case SDLK_HOME:
						emulator_state = 1;
					break;
					default:
					break;
				}
			break;
			#ifdef ENABLE_JOYSTICKCODE
			case SDL_JOYAXISMOTION:
				if (event.jaxis.axis < 2)
				axis_input[event.jaxis.axis] = event.jaxis.value;
			break;
			#endif
		}
	}

	// UP
	if (keys[option.config_buttons[0] ] == SDL_PRESSED
#ifdef ENABLE_JOYSTICKCODE
	|| axis_input[1] < -joy_commit_range
#endif
	) button |= 64;
	
	// DOWN
	if (keys[option.config_buttons[1] ] == SDL_PRESSED
#ifdef ENABLE_JOYSTICKCODE
	|| axis_input[1] > joy_commit_range
#endif
	) button |= 128;
	
	// LEFT
	if (keys[option.config_buttons[2] ] == SDL_PRESSED
#ifdef ENABLE_JOYSTICKCODE
	|| axis_input[0] < -joy_commit_range
#endif
	) button |= 32;
	
	// Right
	if (keys[option.config_buttons[3] ] == SDL_PRESSED
#ifdef ENABLE_JOYSTICKCODE
	|| axis_input[0] > joy_commit_range
#endif
	) button |= 16;
	
	// A
	if (keys[option.config_buttons[4] ] == SDL_PRESSED) button |= 1;
	// B
	if (keys[option.config_buttons[5] ] == SDL_PRESSED) button |= 2;
	
	// L
	if (keys[option.config_buttons[6] ] == SDL_PRESSED) button |= 512;
	// R
	if (keys[option.config_buttons[7] ] == SDL_PRESSED) button |= 256;

	// Start
	if (keys[option.config_buttons[8] ] == SDL_PRESSED) button |= 8;
	// Select
	if (keys[option.config_buttons[9] ] == SDL_PRESSED) button |= 4;
	
	
	return button;
}

