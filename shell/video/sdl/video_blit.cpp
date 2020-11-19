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
#include "globals.h"

#ifdef ENABLE_JOYSTICKCODE
static SDL_Joystick *sdl_joy;
#endif

#ifndef SDL_TRIPLEBUF
#define SDL_TRIPLEBUF SDL_DOUBLEBUF
#endif

#ifndef VIRTUAL_SURFACE
#error "VIRTUAL_SURFACE needs to be defined. Redo the compilation"
#endif

SDL_Surface *sdl_screen, *backbuffer;

uint32_t width_of_surface;

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
	
	sdl_screen = SDL_SetVideoMode(HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, SDL_HWSURFACE | SDL_TRIPLEBUF);
	
	backbuffer = SDL_CreateRGBSurface(SDL_SWSURFACE, HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, 0,0,0,0);
	
	Set_Video_InGame();
}

void Set_Video_Menu()
{
	if (sdl_screen->w != HOST_WIDTH_RESOLUTION)
	{
		//memcpy(pix, sdl_screen->pixels, (INTERNAL_GBA_WIDTH * INTERNAL_GBA_HEIGHT)*2);
		sdl_screen = SDL_SetVideoMode(HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, SDL_HWSURFACE);
	}
}

void Set_Video_InGame()
{
	switch(option.fullscreen) 
	{
        default:
			if (sdl_screen->w != HOST_WIDTH_RESOLUTION) sdl_screen = SDL_SetVideoMode(HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, SDL_HWSURFACE);
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
}

void Update_Video_Menu()
{
	SDL_Flip(sdl_screen);
}


void Update_Video_Ingame(void)
{
	uint_fast16_t y, pitch;
	uint32_t internal_width, internal_height;
	uint16_t *source_graph, *src, *dst;

	internal_width = INTERNAL_GBA_WIDTH;
	internal_height = INTERNAL_GBA_HEIGHT;
	source_graph = (uint16_t* __restrict__)pix;
	
	SDL_LockSurface(sdl_screen);
	
	switch(option.fullscreen) 
	{
		// Fullscreen
		case 0:
			gba_upscale((uint16_t __restrict__*)sdl_screen->pixels, (uint16_t __restrict__*)source_graph, 240, 160, PIX_BUFFER_SCREEN_WIDTH*2, sdl_screen->pitch);
		break;
		case 1:
			gba_upscale_aspect((uint16_t __restrict__*)sdl_screen->pixels + (480*8), (uint16_t __restrict__*)source_graph, 240, 160, PIX_BUFFER_SCREEN_WIDTH*2, sdl_screen->pitch);
		break;
		case 2:
			pitch = HOST_WIDTH_RESOLUTION;
			src = (uint16_t* __restrict__)source_graph;
			dst = (uint16_t* __restrict__)sdl_screen->pixels
				+ ((HOST_WIDTH_RESOLUTION - internal_width) / 4) * sizeof(uint16_t)
				+ ((HOST_HEIGHT_RESOLUTION - internal_height) / 2) * pitch;
			for (y = 0; y < internal_height; y++)
			{
				memmove(dst, src, internal_width * sizeof(uint16_t));
				src += PIX_BUFFER_SCREEN_WIDTH;
				dst += pitch;
			}
		break;
		// Hqx
		case 3:
		break;
	}
	SDL_UnlockSurface(sdl_screen);	
	SDL_Flip(sdl_screen);
}
