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

void Init_Video()
{
	SDL_Init( SDL_INIT_VIDEO );
	
	SDL_ShowCursor(0);
	
	sdl_screen = SDL_SetVideoMode(HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 16, SDL_HWSURFACE);
	
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
			//Draw_to_Virtual_Screen = (uint16_t*)pix;
			width_of_surface = INTERNAL_GBA_WIDTH;
        break;
    }
}

void Close_Video()
{
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
	uint32_t internal_width, internal_height, keep_aspect_width, keep_aspect_height;
	uint16_t*  source_graph;

	internal_width = INTERNAL_GBA_WIDTH;
	internal_height = INTERNAL_GBA_HEIGHT;
	source_graph = (uint16_t* __restrict__)pixels;
	keep_aspect_width = 320;
	keep_aspect_height = 213;
	
	SDL_LockSurface(sdl_screen);
	
	switch(option.fullscreen) 
	{
		case 0:
			bitmap_scale(0,0,internal_width,internal_height,240,160, 256, HOST_WIDTH_RESOLUTION - 240,(uint16_t* __restrict__)source_graph,(uint16_t* __restrict__)sdl_screen->pixels+(HOST_WIDTH_RESOLUTION-240)/2+(HOST_HEIGHT_RESOLUTION-160)/2*HOST_WIDTH_RESOLUTION);
		break;
		// Fullscreen
		case 1:
			bitmap_scale(0, 0, internal_width, internal_height, HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, 256, 0, (uint16_t* __restrict__)source_graph, (uint16_t* __restrict__)sdl_screen->pixels);
		break;
		case 2:
			bitmap_scale(0,0,internal_width,internal_height,keep_aspect_width,keep_aspect_height, 256, HOST_WIDTH_RESOLUTION - keep_aspect_width,(uint16_t* __restrict__)source_graph,(uint16_t* __restrict__)sdl_screen->pixels+(HOST_WIDTH_RESOLUTION-keep_aspect_width)/2+(HOST_HEIGHT_RESOLUTION-keep_aspect_height)/2*HOST_WIDTH_RESOLUTION);
		break;
		// Hqx
		case 3:
		break;
	}
	SDL_UnlockSurface(sdl_screen);	
	SDL_Flip(sdl_screen);
}
