#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <libgen.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <SDL/SDL.h>

#include "scaler.h"
#include "font_drawing.h"
#include "sound_output.h"
#include "video_blit.h"
#include "config.h"
#include "menu.h"
#include "gba.h"

t_config option;
uint32_t emulator_state = 0;

static char home_path[256], save_path[256], eeprom_path[256], conf_path[256];
static uint32_t controls_chosen = 0;

extern SDL_Surface *sdl_screen;
extern char GameName_emu[256];

extern void EEPROM_file(char* path, uint_fast8_t state);
extern void SaveState(char* path, uint_fast8_t state);

static uint8_t selectpressed = 0;
static uint8_t save_slot = 0;

#ifdef IPU_SCALE
#define IPU_OFFSET 1
#define IPU_OFFSET_Y 20
static const int8_t upscalers_available = 1
#else
#define IPU_OFFSET 0
#define IPU_OFFSET_Y 0
static const int8_t upscalers_available = 2
#endif
#ifdef SCALE2X_UPSCALER
+1
#endif
;

static void SaveState_Menu(uint_fast8_t load_mode, uint_fast8_t slot)
{
	char tmp[512];
	snprintf(tmp, sizeof(tmp), "%s/%s_%d.sts", save_path, GameName_emu, slot);
	SaveState(tmp,load_mode);
}

void EEPROM_Menu(uint_fast8_t load_mode)
{
	extern void adjust_save_ram();
	char tmp[512];
	snprintf(tmp, sizeof(tmp), "%s/%s.eps", eeprom_path, GameName_emu);
	adjust_save_ram();
	EEPROM_file(tmp,load_mode);
}

static void config_load()
{
	uint_fast8_t i;
	char config_path[512];
	FILE* fp;
	snprintf(config_path, sizeof(config_path), "%s/%s.cfg", conf_path, GameName_emu);

	fp = fopen(config_path, "rb");
	if (fp)
	{
		fread(&option, sizeof(option), sizeof(int8_t), fp);
		fclose(fp);
	}
	else
	{
		/* Default mapping for Horizontal */
		option.config_buttons[0] = SDLK_UP;
		option.config_buttons[1] = SDLK_DOWN;
		option.config_buttons[2] = SDLK_LEFT;
		option.config_buttons[3] = SDLK_RIGHT;
		
		option.config_buttons[4] = SDLK_LCTRL;
		option.config_buttons[5] = SDLK_LALT;
		option.config_buttons[6] = SDLK_TAB;
		option.config_buttons[7] = SDLK_BACKSPACE;
		
		option.config_buttons[8] = SDLK_RETURN;
		option.config_buttons[9] = SDLK_ESCAPE;
				
		option.config_buttons[10] = 306;
		option.config_buttons[11] = 308;

		/* Set default to keep aspect */
		option.fullscreen = 2;
		#if USE_FRAME_SKIP
		option.frameskip = 5;
		#else
		option.frameskip = 1;
		#endif
	}
}

static void config_save()
{
	FILE* fp;
	char config_path[512];
	snprintf(config_path, sizeof(config_path), "%s/%s.cfg", conf_path, GameName_emu);
	
	fp = fopen(config_path, "wb");
	if (fp)
	{
		fwrite(&option, sizeof(option), sizeof(int8_t), fp);
		fclose(fp);
	}
}

static const char* Return_Text_Button(uint32_t button)
{
	switch(button)
	{
		/* UP button */
		case 273:
			return "DPAD UP";
		break;
		/* DOWN button */
		case 274:
			return "DPAD DOWN";
		break;
		/* LEFT button */
		case 276:
			return "DPAD LEFT";
		break;
		/* RIGHT button */
		case 275:
			return "DPAD RIGHT";
		break;
		/* A button */
		case 306:
			return "A button";
		break;
		/* B button */
		case 308:
			return "B button";
		break;
		/* X button */
		case 304:
			return "X button";
		break;
		/* Y button */
		case 32:
			return "Y button";
		break;
		/* L button */
		case 9:
			return "L button";
		break;
		/* R button */
		case 8:
			return "R button";
		break;
		/* Power button */
		case 279:
			return "L2 button";
		break;
		/* Brightness */
		case 51:
			return "R2 button";
		break;
		/* Volume - */
		case 38:
			return "Volume -";
		break;
		/* Volume + */
		case 233:
			return "Volume +";
		break;
		/* Start */
		case 13:
			return "Start";
		break;
		/* Select */
		case 27:
			return "Select";
		break;
		default:
			return "Unknown key";
		break;
		case 0:
			return "...";
		break;
	}	
}

static void Input_Remapping()
{
	SDL_Event Event;
	char text[50];
	uint32_t pressed = 0;
	int32_t currentselection = 1;
	int32_t exit_input = 0;
	uint32_t exit_map = 0;
	
	while(!exit_input)
	{
		pressed = 0;
		SDL_FillRect( backbuffer, NULL, 0 );
		
        while (SDL_PollEvent(&Event))
        {
            if (Event.type == SDL_KEYDOWN)
            {
                switch(Event.key.keysym.sym)
                {
                    case SDLK_UP:
                        currentselection--;
                        if (currentselection < 1)
                        {
							if (currentselection > 9) currentselection = 12;
							else currentselection = 9;
						}
                        break;
                    case SDLK_DOWN:
                        currentselection++;
                        if (currentselection == 10)
                        {
							currentselection = 1;
						}
                        break;
                    case SDLK_LCTRL:
                    case SDLK_RETURN:
                        pressed = 1;
					break;
                    case SDLK_ESCAPE:
                        option.config_buttons[currentselection - 1] = 0;
					break;
                    case SDLK_LALT:
                        exit_input = 1;
					break;
                    case SDLK_LEFT:
						if (currentselection > 9) currentselection -= 9;
					break;
                    case SDLK_RIGHT:
						if (currentselection < 10) currentselection += 9;
					break;
                    case SDLK_BACKSPACE:
						controls_chosen = 1;
					break;
                    case SDLK_TAB:
						controls_chosen = 0;
					break;
					default:
					break;
                }
            }
        }

        if (pressed)
        {
			SDL_Delay(1);
            switch(currentselection)
            {
                default:
					exit_map = 0;
					while( !exit_map )
					{
						SDL_FillRect( backbuffer, NULL, 0 );
						print_string("Please press button for mapping", TextWhite, TextBlue, 37, 108, (uint16_t*) backbuffer->pixels);
						bitmap_scale(0,0,320,240,HOST_WIDTH_RESOLUTION,HOST_HEIGHT_RESOLUTION,320,0,(uint16_t* )backbuffer->pixels,(uint16_t* )sdl_screen->pixels);
						
						while (SDL_PollEvent(&Event))
						{
							if (Event.type == SDL_KEYDOWN)
							{
								if (Event.key.keysym.sym != SDLK_RCTRL)
								{
									option.config_buttons[currentselection - 1] = Event.key.keysym.sym;
									exit_map = 1;
								}
							}
						}
						SDL_Flip(sdl_screen);
					}
				break;
            }
        }
        
        if (currentselection > 12) currentselection = 12;

		print_string("Press [A] to map to a button", TextWhite, TextBlue, 50, 210, (uint16_t*) backbuffer->pixels);
		print_string("Press [B] to Exit", TextWhite, TextBlue, 85, 225, (uint16_t*) backbuffer->pixels);
		
		snprintf(text, sizeof(text), "UP   : %s\n", Return_Text_Button(option.config_buttons[0]));
		if (currentselection == 1) print_string(text, TextRed, 0, 5, 25+2, (uint16_t*) backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 25+2, (uint16_t*) backbuffer->pixels);
		
		snprintf(text, sizeof(text), "DOWN   : %s\n", Return_Text_Button(option.config_buttons[1]));
		if (currentselection == 2) print_string(text, TextRed, 0, 5, 45+2, (uint16_t*) backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 45+2, (uint16_t*) backbuffer->pixels);
		
		snprintf(text, sizeof(text), "LEFT   : %s\n", Return_Text_Button(option.config_buttons[2]));
		if (currentselection == 3) print_string(text, TextRed, 0, 5, 65+2, (uint16_t*) backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 65+2, (uint16_t*) backbuffer->pixels);
		
		snprintf(text, sizeof(text), "RIGHT   : %s\n", Return_Text_Button(option.config_buttons[3]));
		if (currentselection == 4) print_string(text, TextRed, 0, 5, 85+2, (uint16_t*) backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 85+2, (uint16_t*) backbuffer->pixels);
		
		snprintf(text, sizeof(text), "A   : %s\n", Return_Text_Button(option.config_buttons[4]));
		if (currentselection == 5) print_string(text, TextRed, 0, 5, 105+2, (uint16_t*) backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 105+2, (uint16_t*) backbuffer->pixels);
		
		snprintf(text, sizeof(text), "B   : %s\n", Return_Text_Button(option.config_buttons[5]));
		if (currentselection == 6) print_string(text, TextRed, 0, 5, 125+2, (uint16_t*) backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 125+2, (uint16_t*) backbuffer->pixels);
		
		snprintf(text, sizeof(text), "L   : %s\n", Return_Text_Button(option.config_buttons[6]));
		if (currentselection == 7) print_string(text, TextRed, 0, 5, 145+2, (uint16_t*) backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 145+2, (uint16_t*) backbuffer->pixels);
		
		snprintf(text, sizeof(text), "R   : %s\n", Return_Text_Button(option.config_buttons[7]));
		if (currentselection == 8) print_string(text, TextRed, 0, 5, 165+2, (uint16_t*) backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 165+2, (uint16_t*) backbuffer->pixels);
			
		snprintf(text, sizeof(text), "START : %s\n", Return_Text_Button(option.config_buttons[8]));
		if (currentselection == 9) print_string(text, TextRed, 0, 5, 185+2, (uint16_t*) backbuffer->pixels);
		else print_string(text, TextWhite, 0, 5, 185+2, (uint16_t*) backbuffer->pixels);
			
		snprintf(text, sizeof(text), "SELECT: %s\n", Return_Text_Button(option.config_buttons[9]));
		if (currentselection == 10) print_string(text, TextRed, 0, 165, 25+2, (uint16_t*) backbuffer->pixels);
		else print_string(text, TextWhite, 0, 165, 25+2, (uint16_t*) backbuffer->pixels);
	
		bitmap_scale(0,0,320,240,HOST_WIDTH_RESOLUTION,HOST_HEIGHT_RESOLUTION,320,0,(uint16_t* )backbuffer->pixels,(uint16_t* )sdl_screen->pixels);
		SDL_Flip(sdl_screen);
	}
	
	config_save();
}

#ifdef USE_FRAME_SKIP
#define EXIT_NUMBER 7-IPU_OFFSET
#else
#define EXIT_NUMBER 6-IPU_OFFSET
#endif

void Menu()
{
	char text[50];
    int16_t pressed = 0;
    int16_t currentselection = 1;
    SDL_Rect dstRect;
    SDL_Event Event;
    
    Set_Video_Menu();
    
	/* Save eeprom settings each time we bring up the menu */
	EEPROM_Menu(0);
	
	if (option.fullscreen < 0 || option.fullscreen > upscalers_available) option.fullscreen = 0;
    
    while (((currentselection != 1) && (currentselection != EXIT_NUMBER)) || (!pressed))
    {
        pressed = 0;
        
        SDL_FillRect( backbuffer, NULL, 0 );

		print_string("VBA-Next - Built on " __DATE__, TextWhite, 0, 5, 15, (uint16_t*) backbuffer->pixels);
		
		print_string("Continue", (currentselection == 1) ? TextRed : TextWhite, 0, 5, 45, (uint16_t*) backbuffer->pixels);
		
		snprintf(text, sizeof(text), "Load State %d", save_slot);
		
		print_string(text, (currentselection == 2) ? TextRed : TextWhite, 0, 5, 65, (uint16_t*) backbuffer->pixels);

		snprintf(text, sizeof(text), "Save State %d", save_slot);
		
		print_string(text, (currentselection == 3) ? TextRed : TextWhite, 0, 5, 85, (uint16_t*) backbuffer->pixels);
		
		#ifndef IPU_SCALE
        switch(option.fullscreen)
		{
			case 0:
				print_string("Scaling : Stretched", (currentselection == 4) ? TextRed : TextWhite, 0, 5, 105, (uint16_t*) backbuffer->pixels);
			break;
			case 1:
				print_string("Scaling : Keep scaled", (currentselection == 4) ? TextRed : TextWhite, 0, 5, 105, (uint16_t*) backbuffer->pixels);
			break;
			case 2:
				print_string("Scaling : Native", (currentselection == 4) ? TextRed : TextWhite, 0, 5, 105, (uint16_t*) backbuffer->pixels);
			break;
			case 3:
				print_string("Scaling : EPX/Scale2x", (currentselection == 4) ? TextRed : TextWhite, 0, 5, 105, (uint16_t*) backbuffer->pixels);
			break;
		}
		#endif
		
		
		print_string("Input remapping", (currentselection == 5-IPU_OFFSET) ? TextRed : TextWhite, 0, 5, 125-IPU_OFFSET_Y, (uint16_t*) backbuffer->pixels);
		
		#ifdef USE_FRAME_SKIP
        switch(option.frameskip)
		{
			case 0:
				print_string("Frameskip : None", (currentselection == 6-IPU_OFFSET) ? TextRed : TextWhite, 0, 5, 145-IPU_OFFSET_Y, (uint16_t*) backbuffer->pixels);
			break;
			case 1:
				print_string("Frameskip : 1/3", (currentselection == 6-IPU_OFFSET) ? TextRed : TextWhite, 0, 5, 145-IPU_OFFSET_Y, (uint16_t*) backbuffer->pixels);
			break;
			case 2:
				print_string("Frameskip : 1/2", (currentselection == 6-IPU_OFFSET) ? TextRed : TextWhite, 0, 5, 145-IPU_OFFSET_Y, (uint16_t*) backbuffer->pixels);
			break;
			case 3:
				print_string("Frameskip : 2", (currentselection == 6-IPU_OFFSET) ? TextRed : TextWhite, 0, 5, 145-IPU_OFFSET_Y, (uint16_t*) backbuffer->pixels);
			break;
			case 4:
				print_string("Frameskip : 3", (currentselection == 6-IPU_OFFSET) ? TextRed : TextWhite, 0, 5, 145-IPU_OFFSET_Y, (uint16_t*) backbuffer->pixels);
			break;
			case 5:
				print_string("Frameskip : 4", (currentselection == 6-IPU_OFFSET) ? TextRed : TextWhite, 0, 5, 145-IPU_OFFSET_Y, (uint16_t*) backbuffer->pixels);
			break;
			case 6:
				print_string("Frameskip : Auto", (currentselection == 6-IPU_OFFSET) ? TextRed : TextWhite, 0, 5, 145-IPU_OFFSET_Y, (uint16_t*) backbuffer->pixels);
			break;
		}
		print_string("Quit", (currentselection == EXIT_NUMBER) ? TextRed : TextWhite, 0, 5, 165-IPU_OFFSET_Y, (uint16_t*) backbuffer->pixels);
		#else
		print_string("Quit", (currentselection == EXIT_NUMBER) ? TextRed : TextWhite, 0, 5, 145-IPU_OFFSET_Y, (uint16_t*) backbuffer->pixels);
		#endif
		
		print_string("libretro fork by gameblabla", TextWhite, 0, 5, 205, (uint16_t*) backbuffer->pixels);
		print_string("Credits: libretro, vbam authors", TextWhite, 0, 5, 225, (uint16_t*) backbuffer->pixels);

        while (SDL_PollEvent(&Event))
        {
            if (Event.type == SDL_KEYDOWN)
            {
                switch(Event.key.keysym.sym)
                {
                    case SDLK_UP:
                        currentselection--;
                        if (currentselection == 0)
                            currentselection = EXIT_NUMBER;
                        break;
                    case SDLK_DOWN:
                        currentselection++;
                        if (currentselection == EXIT_NUMBER+1)
                            currentselection = 1;
                        break;
                    case SDLK_END:
                    case SDLK_RCTRL:
                    case SDLK_LALT:
						pressed = 1;
						currentselection = 1;
						break;
                    case SDLK_LCTRL:
                    case SDLK_RETURN:
                        pressed = 1;
                        break;
                    case SDLK_LEFT:
                        switch(currentselection)
                        {
                            case 2:
                            case 3:
                                if (save_slot > 0) save_slot--;
							break;
							#ifndef IPU_SCALE
                            case 4:
							option.fullscreen--;
							if (option.fullscreen < 0)
								option.fullscreen = upscalers_available;
							break;
							#endif
							#ifdef USE_FRAME_SKIP
							case 6-IPU_OFFSET:
								option.frameskip--;
								if (option.frameskip < 0)
									option.frameskip = 6;
							break;
							#endif
                        }
                        break;
                    case SDLK_RIGHT:
                        switch(currentselection)
                        {
                            case 2:
                            case 3:
                                save_slot++;
								if (save_slot == 10)
									save_slot = 9;
							break;
							#ifndef IPU_SCALE
                            case 4:
                                option.fullscreen++;
                                if (option.fullscreen > upscalers_available)
                                    option.fullscreen = 0;
							break;
							#endif
							#ifdef USE_FRAME_SKIP
							case 6-IPU_OFFSET:
								option.frameskip++;
								if (option.frameskip > 6)
									option.frameskip = 0;
							break;
							#endif
                        }
                        break;
					default:
					break;
                }
            }
            else if (Event.type == SDL_QUIT)
            {
				currentselection = EXIT_NUMBER+1;
				pressed = 1;
			}
        }

        if (pressed)
        {
            switch(currentselection)
            {
				case 5-IPU_OFFSET:
					Input_Remapping();
				break;
				#ifndef IPU_SCALE
                case 4-IPU_OFFSET:
                    option.fullscreen++;
                    if (option.fullscreen > upscalers_available)
                        option.fullscreen = 0;
                    break;
				#endif
                case 2 :
                    SaveState_Menu(1, save_slot);
					currentselection = 1;
                    break;
                case 3 :
					SaveState_Menu(0, save_slot);
					currentselection = 1;
				break;
				default:
				break;
            }
        }

		//bitmap_scale(0,0,320,240,HOST_WIDTH_RESOLUTION,HOST_HEIGHT_RESOLUTION,320,0,(uint16_t* )backbuffer->pixels,(uint16_t* )sdl_screen->pixels);
		SDL_SoftStretch(backbuffer, NULL, sdl_screen, NULL);
		SDL_Flip(sdl_screen);
    }
  
    SDL_FillRect(sdl_screen, NULL, 0);
    SDL_Flip(sdl_screen);
    SDL_FillRect(sdl_screen, NULL, 0);
    SDL_Flip(sdl_screen);
    #ifdef SDL_TRIPLEBUF
    SDL_FillRect(sdl_screen, NULL, 0);
    SDL_Flip(sdl_screen);
    #endif
    
    if (currentselection == EXIT_NUMBER)
    {
        done = 1;
        // Make sure to also save when exiting VBA Next
        config_save();
	}
	
	/* Switch back to emulator core */
	emulator_state = 0;
	Set_Video_InGame();
	
	#ifdef USE_FRAME_SKIP
	SetFrameskip(get_frameskip_code());
	#endif
}

static void Cleanup(void)
{
#ifdef SCALE2X_UPSCALER
	if (scale2x_buf) SDL_FreeSurface(scale2x_buf);
#endif
	if (sdl_screen) SDL_FreeSurface(sdl_screen);
	if (backbuffer) SDL_FreeSurface(backbuffer);

	// Deinitialize audio and video output
	Audio_Close();
	
	SDL_Quit();
}

void Init_Configuration()
{
	snprintf(home_path, sizeof(home_path), "%s/.vbanext", getenv("HOME"));
	
	snprintf(conf_path, sizeof(conf_path), "%s/conf", home_path);
	snprintf(save_path, sizeof(save_path), "%s/sstates", home_path);
	snprintf(eeprom_path, sizeof(eeprom_path), "%s/eeprom", home_path);
	
	/* We check first if folder does not exist. 
	 * Let's only try to create it if so in order to decrease boot times.
	 * */
	
	if (access( home_path, F_OK ) == -1)
	{ 
		mkdir(home_path, 0755);
	}
	
	if (access( save_path, F_OK ) == -1)
	{
		mkdir(save_path, 0755);
	}
	
	if (access( conf_path, F_OK ) == -1)
	{
		mkdir(conf_path, 0755);
	}
	
	if (access( eeprom_path, F_OK ) == -1)
	{
		mkdir(eeprom_path, 0755);
	}
	
	/* Load eeprom file if it exists */
	EEPROM_Menu(1);
	
	config_load();
}
