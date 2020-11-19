#ifndef VIDEO_BLIT_H
#define VIDEO_BLIT_H

#include <SDL/SDL.h>

#define HOST_WIDTH_RESOLUTION 320
#define HOST_HEIGHT_RESOLUTION 240

#define MENU_WIDTH_RESOLUTION 320
#define MENU_HEIGHT_RESOLUTION 240

#define INTERNAL_GBA_WIDTH 240
#define INTERNAL_GBA_HEIGHT 160

extern SDL_Surface *sdl_screen, *backbuffer;

void Init_Video();
void Set_Video_Menu();
void Set_Video_InGame();
void Close_Video();
void Update_Video_Menu();
void Update_Video_Ingame(void);

#endif
