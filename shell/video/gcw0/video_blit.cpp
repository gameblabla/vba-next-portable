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

SDL_Surface *sdl_screen, *backbuffer;
#ifdef INTERNAL_BUFFER_GBA
SDL_Surface *int_gba;
#endif

uint32_t width_of_surface;
uint16_t* Draw_to_Virtual_Screen;

#ifndef SDL_TRIPLEBUF
#define SDL_TRIPLEBUF SDL_DOUBLEBUF
#endif

#ifdef INTERNAL_BUFFER_GBA
#define FLAGS_SDL SDL_HWSURFACE | SDL_TRIPLEBUF
#else
#define FLAGS_SDL SDL_HWSURFACE
#endif

#ifdef ENABLE_JOYSTICKCODE
static SDL_Joystick *sdl_joy;
#endif

#if !IPU_SCALE || INTERNAL_BUFFER_GBA
#error "GCW0 port requires IPU_SCALE to be defined and INTERNAL_BUFFER_GBA to be undefined"
#endif

uint16_t* pix;


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
	
	if (sdl_screen && SDL_MUSTLOCK(sdl_screen)) SDL_UnlockSurface(sdl_screen);
	SDL_ShowCursor(0);
	
	sdl_screen = SDL_SetVideoMode(HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, FLAGS_SDL);
	backbuffer = SDL_CreateRGBSurface(SDL_SWSURFACE, HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, 0,0,0,0);
#ifdef INTERNAL_BUFFER_GBA
	int_gba = SDL_CreateRGBSurface(SDL_SWSURFACE, 240, 160, 16, 0,0,0,0);	
#endif
	
	/* This will avoid having an internal buffer for VBA Next and draw directly to the screen instead.
	 * This saves memory and CPU usage, especially on lower end devices. */

#ifdef INTERNAL_BUFFER_GBA
	pix = (uint16_t*) int_gba->pixels;
#else
	pix = (uint16_t*) sdl_screen->pixels;
#endif

	if (SDL_MUSTLOCK(sdl_screen)) SDL_LockSurface(sdl_screen);
	
	Set_Video_InGame();
}

void Set_Video_Menu()
{
	if (sdl_screen && SDL_MUSTLOCK(sdl_screen)) SDL_UnlockSurface(sdl_screen);
	
	if (sdl_screen->w != HOST_WIDTH_RESOLUTION)
	{
		//memcpy(pix, sdl_screen->pixels, (INTERNAL_GBA_WIDTH * INTERNAL_GBA_HEIGHT)*2);
		sdl_screen = SDL_SetVideoMode(HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, FLAGS_SDL);
	}
	
	if (SDL_MUSTLOCK(sdl_screen)) SDL_LockSurface(sdl_screen);
}

void Set_Video_InGame()
{
	if (sdl_screen && SDL_MUSTLOCK(sdl_screen)) SDL_UnlockSurface(sdl_screen);
		
	sdl_screen = SDL_SetVideoMode(240, 160, 16, FLAGS_SDL);
	width_of_surface = INTERNAL_GBA_WIDTH;
    
	if (SDL_MUSTLOCK(sdl_screen)) SDL_LockSurface(sdl_screen);
	
	#ifndef INTERNAL_BUFFER_GBA
	/* We must make sure to do that as otherwise, the memory address for sdl_screen->pixels can change after calling SDL_SetVideoMode again */
	pix = (uint16_t*) sdl_screen->pixels;
	#endif
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

void Update_Video_Ingame(uint16_t* __restrict__ pixels)
{
#ifdef INTERNAL_BUFFER_GBA
	SDL_BlitSurface(int_gba, NULL, sdl_screen, NULL);
#endif
	SDL_Flip(sdl_screen);	
}
