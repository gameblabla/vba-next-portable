/* Cygne
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Dox dox@space.pl
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <SDL/SDL.h>
#include <sys/time.h>
#include <sys/types.h>

#include "video_blit.h"
#include "scaler.h"
#include "config.h"

SDL_Surface *sdl_screen, *backbuffer;

uint32_t width_of_surface;
uint16_t* Draw_to_Virtual_Screen;
uint_fast8_t aspect_ratio_hw = 0;

#define FLAGS_SDL SDL_HWSURFACE

#ifndef SDL_TRIPLEBUF
#define SDL_TRIPLEBUF SDL_DOUBLEBUF
#endif

#ifdef ENABLE_JOYSTICKCODE
static SDL_Joystick *sdl_joy;
#endif

static const char *KEEP_ASPECT_FILENAME = "/sys/devices/platform/jz-lcd.0/keep_aspect_ratio";

static inline uint_fast8_t get_keep_aspect_ratio()
{
	FILE *f = fopen(KEEP_ASPECT_FILENAME, "rb");
	if (!f) return false;
	char c;
	fread(&c, 1, 1, f);
	fclose(f);
	return c == 'Y';
}

static inline void set_keep_aspect_ratio(uint_fast8_t n)
{
	FILE *f = fopen(KEEP_ASPECT_FILENAME, "wb");
	if (!f) return;
	char c = n ? 'Y' : 'N';
	fwrite(&c, 1, 1, f);
	fclose(f);
}

void Init_Video()
{
	#ifdef ENABLE_JOYSTICKCODE
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
	
	if (SDL_NumJoysticks() > 0)
	{
		sdl_joy = SDL_JoystickOpen(0);
		SDL_JoystickEventState(SDL_ENABLE);
	}
	#else
	SDL_Init(SDL_INIT_VIDEO);
	#endif
	
	SDL_ShowCursor(0);
	
	sdl_screen = SDL_SetVideoMode(HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, FLAGS_SDL);
	backbuffer = SDL_CreateRGBSurface(SDL_SWSURFACE, HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, 0,0,0,0);
	
	aspect_ratio_hw = get_keep_aspect_ratio();
	
	Set_Video_InGame();
}

void Set_Video_Menu()
{
	if (sdl_screen->w != HOST_WIDTH_RESOLUTION)
	{
		//memcpy(pix, sdl_screen->pixels, (INTERNAL_GBA_WIDTH * INTERNAL_GBA_HEIGHT)*2);
		sdl_screen = SDL_SetVideoMode(HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, FLAGS_SDL);
	}
}

void Set_Video_InGame()
{
	/* Failsafe, just in case. Should be moved elsewhere but meh. */
	if (option.fullscreen < 0 || option.fullscreen > 2) option.fullscreen = 1;
	
	switch(option.fullscreen) 
	{
		case 0:
			sdl_screen = SDL_SetVideoMode(HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, FLAGS_SDL);
			width_of_surface = INTERNAL_GBA_WIDTH;
		break;
		/* Stretched, fullscreen*/
		case 1:
			set_keep_aspect_ratio(0);
			sdl_screen = SDL_SetVideoMode(240, 160, 16, FLAGS_SDL);
			width_of_surface = INTERNAL_GBA_WIDTH;
		break;
		/* Keep Aspect Ratio */
		case 2:
			set_keep_aspect_ratio(1);
			sdl_screen = SDL_SetVideoMode(240, 160, 16, FLAGS_SDL);
			width_of_surface = INTERNAL_GBA_WIDTH;
		break;
    }
}

void Close_Video()
{
	#ifdef ENABLE_JOYSTICKCODE
	if (SDL_JoystickOpened(0)) SDL_JoystickClose(sdl_joy);
	#endif
	if (sdl_screen) SDL_FreeSurface(sdl_screen);
	if (backbuffer) SDL_FreeSurface(backbuffer);
	SDL_Quit();
	
	/* Set it back to the Default Setting when entering VBA Next */
	set_keep_aspect_ratio(aspect_ratio_hw);
}

void Update_Video_Menu()
{
	SDL_Flip(sdl_screen);
}


void Update_Video_Ingame(uint16_t* __restrict__ pixels)
{
	uint_fast16_t y, pitch;
	uint16_t *source_graph, *src, *dst;

	source_graph = (uint16_t* __restrict__)pixels;

	SDL_LockSurface(sdl_screen);
	
	pitch = sdl_screen->w;
	src = (uint16_t* __restrict__)source_graph;
	dst = (uint16_t* __restrict__)sdl_screen->pixels
		+ ((sdl_screen->w - INTERNAL_GBA_WIDTH) / 4) * sizeof(uint16_t)
		+ ((sdl_screen->h - INTERNAL_GBA_HEIGHT) / 2) * pitch;
	for (y = 0; y < INTERNAL_GBA_HEIGHT; y++)
	{
		memmove(dst, src, INTERNAL_GBA_WIDTH * sizeof(uint16_t));
		src += 256;
		dst += pitch;
	}
	SDL_UnlockSurface(sdl_screen);	
	SDL_Flip(sdl_screen);
}
