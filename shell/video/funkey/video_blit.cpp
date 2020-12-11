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

SDL_Surface *sdl_screen, *backbuffer, *gba_screen;
#ifndef SDL_TRIPLEBUF
#warning "Triple buffering not available : Reverting back to Double buffering"
#define SDL_TRIPLEBUF SDL_DOUBLEBUF
#endif

static SDL_Rect rc = {0, 20, 240, 200};

#define FLAGS_SDL SDL_HWSURFACE | SDL_DOUBLEBUF

void Init_Video()
{
	SDL_Init(SDL_INIT_VIDEO);
	
	if (sdl_screen && SDL_MUSTLOCK(sdl_screen)) SDL_UnlockSurface(sdl_screen);
	SDL_ShowCursor(0);
	sdl_screen = SDL_SetVideoMode(240, 240, 16, FLAGS_SDL);
	backbuffer = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 240, 16, 0,0,0,0);
	gba_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, 240, 160, 16, 0,0,0,0);
	if (SDL_MUSTLOCK(sdl_screen)) SDL_LockSurface(sdl_screen);
	Set_Video_InGame();
}

void Set_Video_Menu()
{
}

void Set_Video_InGame()
{
	switch(option.fullscreen) 
	{
		// Fullscreen
		case 0:
		case 1:
			pix = (uint16_t*) gba_screen->pixels;	
		break;
		default:
			pix = (uint16_t*) sdl_screen->pixels + ((20*240)*2);	
		break;
	}
}

void Clean_Video()
{
	for(uint_fast8_t i=0;i<3;i++)
	{
		SDL_FillRect(gba_screen, NULL, 0);
		SDL_FillRect(backbuffer, NULL, 0);
		SDL_FillRect(sdl_screen, NULL, 0);
		SDL_Flip(sdl_screen);
	}	
}

void Close_Video()
{
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
	if (gba_screen)
	{
		SDL_FreeSurface(gba_screen);
		gba_screen = NULL;
	}
	SDL_Quit();
}


void Update_Video_Menu()
{
	SDL_SoftStretch(backbuffer, NULL, sdl_screen, NULL);
	SDL_Flip(sdl_screen);
}

void Update_Video_Ingame(void)
{
	switch(option.fullscreen) 
	{
		// Fullscreen
		case 0:
			upscale_160x240_to_240x240_bilinearish((uint16_t* __restrict__)gba_screen->pixels, (uint16_t* __restrict__)sdl_screen->pixels);
		break;
		case 1:
			SDL_SoftStretch(gba_screen, NULL, sdl_screen, &rc);
		break;
		default:
		break;
	}
	SDL_Flip(sdl_screen);	
}
