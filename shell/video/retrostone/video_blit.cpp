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

#ifndef VIRTUAL_SURFACE
#error "VIRTUAL_SURFACE needs to be defined. Redo the compilation"
#endif

SDL_Surface *sdl_screen, *backbuffer;

uint32_t width_of_surface;
uint16_t* Draw_to_Virtual_Screen;

void Init_Video()
{
	SDL_Init( SDL_INIT_VIDEO );
	
	SDL_ShowCursor(0);
	
	sdl_screen = SDL_SetVideoMode(0, 0, 16, SDL_HWSURFACE);
	
	backbuffer = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 240, 16, 0,0,0,0);
	
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

void Clean_Video()
{
	for(uint_fast8_t i=0;i<3;i++)
	{
		SDL_FillRect(sdl_screen, NULL, 0);
		SDL_Flip(sdl_screen);
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
	SDL_SoftStretch(backbuffer, NULL, sdl_screen, NULL);
	SDL_Flip(sdl_screen);
}


void Update_Video_Ingame(void)
{
	uint32_t internal_width, internal_height, keep_aspect_width, keep_aspect_height;
	uint16_t*  source_graph;

	internal_width = INTERNAL_GBA_WIDTH;
	internal_height = INTERNAL_GBA_HEIGHT;
	source_graph = (uint16_t* __restrict__)pix;
	
	keep_aspect_width = ((HOST_HEIGHT_RESOLUTION / INTERNAL_GBA_HEIGHT) * INTERNAL_GBA_WIDTH) + HOST_WIDTH_RESOLUTION/4;
	if (keep_aspect_width > HOST_WIDTH_RESOLUTION) keep_aspect_width -= HOST_WIDTH_RESOLUTION/4;
	keep_aspect_height = HOST_HEIGHT_RESOLUTION;
	
	SDL_LockSurface(sdl_screen);
	
	switch(option.fullscreen) 
	{
		// Fullscreen
		case 0:
			bitmap_scale(0, 0, internal_width, internal_height, HOST_WIDTH_RESOLUTION, HOST_HEIGHT_RESOLUTION, PIX_BUFFER_SCREEN_WIDTH, 0, (uint16_t* __restrict__)source_graph, (uint16_t* __restrict__)sdl_screen->pixels);
		break;
		case 1:
			bitmap_scale(0,0,internal_width,internal_height,keep_aspect_width,keep_aspect_height, PIX_BUFFER_SCREEN_WIDTH, HOST_WIDTH_RESOLUTION - keep_aspect_width,(uint16_t* __restrict__)source_graph,(uint16_t* __restrict__)sdl_screen->pixels+(HOST_WIDTH_RESOLUTION-keep_aspect_width)/2+(HOST_HEIGHT_RESOLUTION-keep_aspect_height)/2*HOST_WIDTH_RESOLUTION);
		break;
		case 2:
			bitmap_scale(0,0,internal_width,internal_height,240*2,160*2, PIX_BUFFER_SCREEN_WIDTH, HOST_WIDTH_RESOLUTION - 240*2,(uint16_t* __restrict__)source_graph,(uint16_t* __restrict__)sdl_screen->pixels+(HOST_WIDTH_RESOLUTION-240*2)/2+(HOST_HEIGHT_RESOLUTION-160*2)/2*HOST_WIDTH_RESOLUTION);
		break;
		// Hqx
		case 3:
		break;
	}
	SDL_UnlockSurface(sdl_screen);	
	SDL_Flip(sdl_screen);
}
