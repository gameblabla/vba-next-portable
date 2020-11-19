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
#ifndef SDL_TRIPLEBUF
#warning "Triple buffering not available : Reverting back to Double buffering"
#define SDL_TRIPLEBUF SDL_DOUBLEBUF
#endif

#ifdef VIRTUAL_SURFACE
#define FLAGS_SDL SDL_HWSURFACE | SDL_TRIPLEBUF
#else
#define FLAGS_SDL SDL_HWSURFACE
#endif

#ifdef ENABLE_JOYSTICKCODE
static SDL_Joystick *sdl_joy;
#endif

#if !IPU_SCALE
#error "GCW0 port requires IPU_SCALE to be defined"
#endif

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
    
	if (SDL_MUSTLOCK(sdl_screen)) SDL_LockSurface(sdl_screen);
	
	/* We must make sure to do that as otherwise, the memory address for sdl_screen->pixels can change after calling SDL_SetVideoMode again */
	#ifndef VIRTUAL_SURFACE
	pix = (uint16_t*) sdl_screen->pixels;
	#endif
}

void Close_Video()
{
	#ifdef ENABLE_JOYSTICKCODE
	if (SDL_JoystickOpened(0)) SDL_JoystickClose(sdl_joy);
	#endif
	if (sdl_screen)
	{
		SDL_FreeSurface(sdl_screen);
		sdl_screen = NULL;
	}
	if (backbuffer)
	{
		SDL_FreeSurface(backbuffer);
		backbuffer = NULL;
	}
	SDL_Quit();
}


void Update_Video_Menu()
{
	SDL_Flip(sdl_screen);
}

void Update_Video_Ingame(void)
{
#ifdef VIRTUAL_SURFACE
	if (SDL_LockSurface(sdl_screen) == 0)
	{
		memmove(sdl_screen->pixels, pix, (240*160)*2);
		SDL_UnlockSurface(sdl_screen);
	}
#endif
	SDL_Flip(sdl_screen);	
}
