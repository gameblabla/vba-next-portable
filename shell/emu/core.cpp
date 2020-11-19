#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <string.h>
#include <libgen.h>
#include <sys/time.h>

#include "system.h"
#include "port.h"
#include "types.h"
#include "gba.h"
#include "memory.h"
#include "sound.h"
#include "globals.h"

// Shell specific
#include "sound_output.h"
#include "video_blit.h"
#include "input.h"
#include "menu.h"
#include "config.h"
#include "shared.h"

extern uint64_t joy;
unsigned device_type = 0;

char GameName_emu[256];
uint32_t frames = 0 ;
uint32_t done = 0;

char filename_bios[0x100] = {0};

uint8_t libretro_save_buf[0x20000 + 0x2000];	/* Workaround for broken-by-design GBA save semantics. */

static unsigned libretro_save_size = sizeof(libretro_save_buf);

static uint_fast8_t scan_area(const uint8_t *data, unsigned size)
{
   for (unsigned i = 0; i < size; i++)
      if (data[i] != 0xff)
         return 1;

   return 0;
}

void adjust_save_ram()
{
   if (scan_area(libretro_save_buf, 512) &&
         !scan_area(libretro_save_buf + 512, sizeof(libretro_save_buf) - 512))
   {
      libretro_save_size = 512;
   }
   else if (scan_area(libretro_save_buf, 0x2000) && 
         !scan_area(libretro_save_buf + 0x2000, sizeof(libretro_save_buf) - 0x2000))
   {
      libretro_save_size = 0x2000;
   }

   else if (scan_area(libretro_save_buf, 0x10000) && 
         !scan_area(libretro_save_buf + 0x10000, sizeof(libretro_save_buf) - 0x10000))
   {
      libretro_save_size = 0x10000;
   }
   else if (scan_area(libretro_save_buf, 0x20000) && 
         !scan_area(libretro_save_buf + 0x20000, sizeof(libretro_save_buf) - 0x20000))
   {
      libretro_save_size = 0x20000;
   }

   if (libretro_save_size == 512 || libretro_save_size == 0x2000)
      eepromData = libretro_save_buf;
   else if (libretro_save_size == 0x10000 || libretro_save_size == 0x20000)
      flashSaveMemory = libretro_save_buf;
}

void vbanext_init(void)
{
   memset(libretro_save_buf, 0xff, sizeof(libretro_save_buf));
   //adjust_save_ram();

#if THREADED_RENDERER
	ThreadedRendererStart();
#endif
}

static unsigned serialize_size = 0;

typedef struct  {
	char romtitle[256];
	char romid[5];
	int flashSize;
	int saveType;
	int rtcEnabled;
	int mirroringEnabled;
	int useBios;
} ini_t;

static const ini_t gbaover[256] = {
			//romtitle,							    	romid	flash	save	rtc	mirror	bios
			{"2 Games in 1 - Dragon Ball Z - The Legacy of Goku I & II (USA)",	"BLFE",	0,	1,	0,	0,	0},
			{"2 Games in 1 - Dragon Ball Z - Buu's Fury + Dragon Ball GT - Transformation (USA)", "BUFE", 0, 1, 0, 0, 0},
			{"Boktai - The Sun Is in Your Hand (Europe)(En,Fr,De,Es,It)",		"U3IP",	0,	0,	1,	0,	0},
			{"Boktai - The Sun Is in Your Hand (USA)",				"U3IE",	0,	0,	1,	0,	0},
			{"Boktai 2 - Solar Boy Django (USA)",					"U32E",	0,	0,	1,	0,	0},
			{"Boktai 2 - Solar Boy Django (Europe)(En,Fr,De,Es,It)",		"U32P",	0,	0,	1,	0,	0},
			{"Bokura no Taiyou - Taiyou Action RPG (Japan)",			"U3IJ",	0,	0,	1,	0,	0},
			{"Card e-Reader+ (Japan)",						"PSAJ",	131072,	0,	0,	0,	0},
			{"Classic NES Series - Bomberman (USA, Europe)",			"FBME",	0,	1,	0,	1,	0},
			{"Classic NES Series - Castlevania (USA, Europe)",			"FADE",	0,	1,	0,	1,	0},
			{"Classic NES Series - Donkey Kong (USA, Europe)",			"FDKE",	0,	1,	0,	1,	0},
			{"Classic NES Series - Dr. Mario (USA, Europe)",			"FDME",	0,	1,	0,	1,	0},
			{"Classic NES Series - Excitebike (USA, Europe)",			"FEBE",	0,	1,	0,	1,	0},
			{"Classic NES Series - Legend of Zelda (USA, Europe)",			"FZLE",	0,	1,	0,	1,	0},
			{"Classic NES Series - Ice Climber (USA, Europe)",			"FICE",	0,	1,	0,	1,	0},
			{"Classic NES Series - Metroid (USA, Europe)",				"FMRE",	0,	1,	0,	1,	0},
			{"Classic NES Series - Pac-Man (USA, Europe)",				"FP7E",	0,	1,	0,	1,	0},
			{"Classic NES Series - Super Mario Bros. (USA, Europe)",		"FSME",	0,	1,	0,	1,	0},
			{"Classic NES Series - Xevious (USA, Europe)",				"FXVE",	0,	1,	0,	1,	0},
			{"Classic NES Series - Zelda II - The Adventure of Link (USA, Europe)",	"FLBE",	0,	1,	0,	1,	0},
			{"Digi Communication 2 - Datou! Black Gemagema Dan (Japan)",		"BDKJ",	0,	1,	0,	0,	0},
			{"e-Reader (USA)",							"PSAE",	131072,	0,	0,	0,	0},
			{"Dragon Ball GT - Transformation (USA)",				"BT4E",	0,	1,	0,	0,	0},
			{"Dragon Ball Z - Buu's Fury (USA)",					"BG3E",	0,	1,	0,	0,	0},
			{"Dragon Ball Z - Taiketsu (Europe)(En,Fr,De,Es,It)",			"BDBP",	0,	1,	0,	0,	0},
			{"Dragon Ball Z - Taiketsu (USA)",					"BDBE",	0,	1,	0,	0,	0},
			{"Dragon Ball Z - The Legacy of Goku II International (Japan)",		"ALFJ",	0,	1,	0,	0,	0},
			{"Dragon Ball Z - The Legacy of Goku II (Europe)(En,Fr,De,Es,It)",	"ALFP", 0,	1,	0,	0,	0},
			{"Dragon Ball Z - The Legacy of Goku II (USA)",				"ALFE",	0,	1,	0,	0,	0},
			{"Dragon Ball Z - The Legacy Of Goku (Europe)(En,Fr,De,Es,It)",		"ALGP",	0,	1,	0,	0,	0},
			{"Dragon Ball Z - The Legacy of Goku (USA)",				"ALGE",	131072,	1,	0,	0,	0},
			{"F-Zero - Climax (Japan)",						"BFTJ",	131072,	0,	0,	0,	0},
			{"Famicom Mini Vol. 01 - Super Mario Bros. (Japan)",			"FMBJ",	0,	1,	0,	1,	0},
			{"Famicom Mini Vol. 12 - Clu Clu Land (Japan)",				"FCLJ",	0,	1,	0,	1,	0},
			{"Famicom Mini Vol. 13 - Balloon Fight (Japan)",			"FBFJ",	0,	1,	0,	1,	0},
			{"Famicom Mini Vol. 14 - Wrecking Crew (Japan)",			"FWCJ",	0,	1,	0,	1,	0},
			{"Famicom Mini Vol. 15 - Dr. Mario (Japan)",				"FDMJ",	0,	1,	0,	1,	0},
			{"Famicom Mini Vol. 16 - Dig Dug (Japan)",				"FTBJ",	0,	1,	0,	1,	0},
			{"Famicom Mini Vol. 17 - Takahashi Meijin no Boukenjima (Japan)",	"FTBJ",	0,	1,	0,	1,	0},
			{"Famicom Mini Vol. 18 - Makaimura (Japan)",				"FMKJ",	0,	1,	0,	1,	0},
			{"Famicom Mini Vol. 19 - Twin Bee (Japan)",				"FTWJ",	0,	1,	0,	1,	0},
			{"Famicom Mini Vol. 20 - Ganbare Goemon! Karakuri Douchuu (Japan)",	"FGGJ",	0,	1,	0,	1,	0},
			{"Famicom Mini Vol. 21 - Super Mario Bros. 2 (Japan)",			"FM2J",	0,	1,	0,	1,	0},
			{"Famicom Mini Vol. 22 - Nazo no Murasame Jou (Japan)",			"FNMJ",	0,	1,	0,	1,	0},
			{"Famicom Mini Vol. 23 - Metroid (Japan)",				"FMRJ",	0,	1,	0,	1,	0},
			{"Famicom Mini Vol. 24 - Hikari Shinwa - Palthena no Kagami (Japan)",	"FPTJ",	0,	1,	0,	1,	0},
			{"Famicom Mini Vol. 25 - The Legend of Zelda 2 - Link no Bouken (Japan)","FLBJ",0,	1,	0,	1,	0},
			{"Famicom Mini Vol. 26 - Famicom Mukashi Banashi - Shin Onigashima - Zen Kou Hen (Japan)","FFMJ",0,1,0,	1,	0},
			{"Famicom Mini Vol. 27 - Famicom Tantei Club - Kieta Koukeisha - Zen Kou Hen (Japan)","FTKJ",0,1,0,	1,	0},
			{"Famicom Mini Vol. 28 - Famicom Tantei Club Part II - Ushiro ni Tatsu Shoujo - Zen Kou Hen (Japan)","FTUJ",0,1,0,1,0},
			{"Famicom Mini Vol. 29 - Akumajou Dracula (Japan)",			"FADJ",	0,	1,	0,	1,	0},
			{"Famicom Mini Vol. 30 - SD Gundam World - Gachapon Senshi Scramble Wars (Japan)","FSDJ",0,1,	0,	1,	0},
			{"Game Boy Wars Advance 1+2 (Japan)",					"BGWJ",	131072,	0,	0,	0,	0},
			{"Golden Sun - The Lost Age (USA)",					"AGFE",	65536,	0,	0,	1,	0},
			{"Golden Sun (USA)",							"AGSE",	65536,	0,	0,	1,	0},
			{"Iridion II (Europe) (En,Fr,De)",							"AI2P",	0,	5,	0,	0,	0},
			{"Iridion II (USA)",							"AI2E",	0,	5,	0,	0,	0},
			{"Koro Koro Puzzle - Happy Panechu! (Japan)",				"KHPJ",	0,	4,	0,	0,	0},
			{"Mario vs. Donkey Kong (Europe)",					"BM5P",	0,	3,	0,	0,	0},
			{"Pocket Monsters - Emerald (Japan)",					"BPEJ",	131072,	0,	1,	0,	0},
			{"Pocket Monsters - Fire Red (Japan)",					"BPRJ",	131072,	0,	0,	0,	0},
			{"Pocket Monsters - Leaf Green (Japan)",				"BPGJ",	131072,	0,	0,	0,	0},
			{"Pocket Monsters - Ruby (Japan)",					"AXVJ",	131072,	0,	1,	0,	0},
			{"Pocket Monsters - Sapphire (Japan)",					"AXPJ",	131072,	0,	1,	0,	0},
			{"Pokemon Mystery Dungeon - Red Rescue Team (USA, Australia)",		"B24E",	131072,	0,	0,	0,	0},
			{"Pokemon Mystery Dungeon - Red Rescue Team (En,Fr,De,Es,It)",		"B24P",	131072,	0,	0,	0,	0},
			{"Pokemon - Blattgruene Edition (Germany)",				"BPGD",	131072,	0,	0,	0,	0},
			{"Pokemon - Edicion Rubi (Spain)",					"AXVS",	131072,	0,	1,	0,	0},
			{"Pokemon - Edicion Esmeralda (Spain)",					"BPES",	131072,	0,	1,	0,	0},
			{"Pokemon - Edicion Rojo Fuego (Spain)",				"BPRS",	131072,	1,	0,	0,	0},
			{"Pokemon - Edicion Verde Hoja (Spain)",				"BPGS",	131072,	1,	0,	0,	0},
			{"Pokemon - Eidicion Zafiro (Spain)",					"AXPS",	131072,	0,	1,	0,	0},
			{"Pokemon - Emerald Version (USA, Europe)",				"BPEE",	131072,	0,	1,	0,	0},
			{"Pokemon - Feuerrote Edition (Germany)",				"BPRD",	131072,	0,	0,	0,	0},
			{"Pokemon - Fire Red Version (USA, Europe)",				"BPRE",	131072,	0,	0,	0,	0},
			{"Pokemon - Leaf Green Version (USA, Europe)",				"BPGE",	131072,	0,	0,	0,	0},
			{"Pokemon - Rubin Edition (Germany)",					"AXVD",	131072,	0,	1,	0,	0},
			{"Pokemon - Ruby Version (USA, Europe)",				"AXVE",	131072,	0,	1,	0,	0},
			{"Pokemon - Sapphire Version (USA, Europe)",				"AXPE",	131072,	0,	1,	0,	0},
			{"Pokemon - Saphir Edition (Germany)",					"AXPD",	131072,	0,	1,	0,	0},
			{"Pokemon - Smaragd Edition (Germany)",					"BPED",	131072,	0,	1,	0,	0},
			{"Pokemon - Version Emeraude (France)",					"BPEF",	131072,	0,	1,	0,	0},
			{"Pokemon - Version Rouge Feu (France)",				"BPRF",	131072,	0,	0,	0,	0},
			{"Pokemon - Version Rubis (France)",					"AXVF",	131072,	0,	1,	0,	0},
			{"Pokemon - Version Saphir (France)",					"AXPF",	131072,	0,	1,	0,	0},
			{"Pokemon - Version Vert Feuille (France)",				"BPGF",	131072,	0,	0,	0,	0},
			{"Pokemon - Versione Rubino (Italy)",					"AXVI",	131072,	0,	1,	0,	0},
			{"Pokemon - Versione Rosso Fuoco (Italy)",				"BPRI",	131072,	0,	0,	0,	0},
			{"Pokemon - Versione Smeraldo (Italy)",					"BPEI",	131072,	0,	1,	0,	0},
			{"Pokemon - Versione Verde Foglia (Italy)",				"BPGI",	131072,	0,	0,	0,	0},
			{"Pokemon - Versione Zaffiro (Italy)",					"AXPI",	131072,	0,	1,	0,	0},
			{"Rockman EXE 4.5 - Real Operation (Japan)",				"BR4J",	0,	0,	1,	0,	0},
			{"Rocky (Europe)(En,Fr,De,Es,It)",					"AROP",	0,	1,	0,	0,	0},
			{"Rocky (USA)(En,Fr,De,Es,It)",						"AR8e",	0,	1,	0,	0,	0},
			{"Sennen Kazoku (Japan)",						"BKAJ",	131072,	0,	1,	0,	0},
			{"Shin Bokura no Taiyou - Gyakushuu no Sabata (Japan)",			"U33J",	0,	1,	1,	0,	0},
			{"Super Mario Advance 4 (Japan)",					"AX4J",	131072,	0,	0,	0,	0},
			{"Super Mario Advance 4 - Super Mario Bros. 3 (Europe)(En,Fr,De,Es,It)","AX4P",	131072,	0,	0,	0,	0},
			{"Super Mario Advance 4 - Super Mario Bros 3 - Super Mario Advance 4 v1.1 (USA)","AX4E",131072,0,0,0,0},
			{"Top Gun - Combat Zones (USA)(En,Fr,De,Es,It)",			"A2YE",	0,	5,	0,	0,	0},
			{"Yoshi's Universal Gravitation (Europe)(En,Fr,De,Es,It)",		"KYGP",	0,	4,	0,	0,	0},
			{"Yoshi no Banyuuinryoku (Japan)",					"KYGJ",	0,	4,	0,	0,	0},
			{"Yoshi - Topsy-Turvy (USA)",						"KYGE",	0,	1,	0,	0,	0},
			{"Yu-Gi-Oh! GX - Duel Academy (USA)",					"BYGE",	0,	2,	0,	0,	1},
			{"Yu-Gi-Oh! - Ultimate Masters - 2006 (Europe)(En,Jp,Fr,De,Es,It)",	"BY6P",	0,	2,	0,	0,	0},
			{"Zoku Bokura no Taiyou - Taiyou Shounen Django (Japan)",		"U32J",	0,	0,	1,	0,	0}
};

static void load_image_preferences (void)
{
	char buffer[5];
	buffer[0] = rom[0xac];
	buffer[1] = rom[0xad];
	buffer[2] = rom[0xae];
	buffer[3] = rom[0xaf];
	buffer[4] = 0;

	uint_fast8_t found = 0;
	int found_no = 0;

	for(int i = 0; i < 256; i++)
	{
		if(!strcmp(gbaover[i].romid, buffer))
		{
			found = 1;
			found_no = i;
         break;
		}
	}

	if(found)
	{
		enableRtc = gbaover[found_no].rtcEnabled;

		if(gbaover[found_no].flashSize != 0)
			flashSize = gbaover[found_no].flashSize;
		else
			flashSize = 65536;

		cpuSaveType = gbaover[found_no].saveType;

		mirroringEnable = gbaover[found_no].mirroringEnabled;
	}
}

#if USE_FRAME_SKIP
int get_frameskip_code()
{
	/*if (strcmp(var.value, "1/3") == 0) return 0x13;
	if (strcmp(var.value, "1/2") == 0) return 0x12;
	if (strcmp(var.value, "1") == 0) return 0x1;
	if (strcmp(var.value, "2") == 0) return 0x2;
	if (strcmp(var.value, "3") == 0) return 0x3;
	if (strcmp(var.value, "4") == 0) return 0x4;*/
	switch(option.frameskip)
	{
		case 1:
			return 0x13;
		break;
		case 2:
			return 0x12;
		break;
		case 3:
			return 0x2;
		break;
		case 4:
			return 0x3;
		break;
		case 5:
			return 0x4;
		break;
		default:
			return 0x0;
		break;
	}
	return 0x0;
}
#endif

static void gba_init(void)
{
   cpuSaveType = 0;
   flashSize = 0x10000;
   enableRtc = 0;
   mirroringEnable = 0;

   load_image_preferences();

   if(flashSize == 0x10000 || flashSize == 0x20000)
      flashSetSize(flashSize);

   if(enableRtc)
      rtcEnable(enableRtc);

   doMirroring(mirroringEnable);

   soundSetSampleRate(SOUND_OUTPUT_FREQUENCY);

   CPUInit(NULL, 0);
   CPUReset();

   soundReset();

   uint8_t * state_buf = (uint8_t*)malloc(2000000);
   serialize_size = CPUWriteState(state_buf, 2000000);
   free(state_buf);

#if USE_FRAME_SKIP
	SetFrameskip(get_frameskip_code());
#endif
}

void vbanext_deinit(void)
{
#if THREADED_RENDERER
	ThreadedRendererStop();
#endif
	CPUCleanUp();
}

void vbanext_reset(void)
{
   CPUReset();
}

static unsigned has_frame;

/*
#if USE_FRAME_SKIP
   SetFrameskip(get_frameskip_code());
#endif
*/

#ifdef USE_FRAME_SKIP
static uint32_t Timer_Read(void) 
{
	/* Timing. */
	struct timeval tval;
  	gettimeofday(&tval, 0);
	return (((tval.tv_sec*1000000) + (tval.tv_usec)));
}
static long lastTick = 0, newTick;
static uint32_t video_frames = 0, FPS = 60, FrameSkip = 0;
#endif

void vbanext_run(void)
{
   joy = update_input();

   has_frame = 0;
   UpdateJoypad();
   
   do
   {
      CPULoop();
   }while (!has_frame);
   
#ifdef USE_FRAME_SKIP
	if (option.frameskip == 6)
	{
		video_frames++;
		newTick = Timer_Read();
		if ( (newTick) - (lastTick) > 1000000) 
		{
			FPS = video_frames;
			video_frames = 0;
			lastTick = newTick;
			
			if (FPS > 58)
			{
				FrameSkip = 0;
				SetFrameskip(0);
			}
			else
			{
				if (FPS > 44) SetFrameskip(0x1);
				else if (FPS > 29) SetFrameskip(0x2);
				else if (FPS > 14) SetFrameskip(0x3);
				else SetFrameskip(0x4);
			}
		}
	}
#elif defined(FORCE_FRAMESKIP)
	SetFrameskip(FORCE_FRAMESKIP);
#endif
}

void vbanext_cheat_reset(void)
{
   cheatsDeleteAll(0);
}

#define ISHEXDEC \
   ((code[cursor] >= '0') && (code[cursor] <= '9')) || \
   ((code[cursor] >= 'a') && (code[cursor] <= 'f')) || \
   ((code[cursor] >= 'A') && (code[cursor] <= 'F')) \

void vbanext_cheat_set(unsigned index, uint_fast8_t enabled, const char *code)
{
   char name[128];
   unsigned cursor;
   char *codeLine = NULL ;
   int codeLineSize = strlen(code)+5 ;
   int codePos = 0 ;
   int i ;

   codeLine = (char*)calloc(codeLineSize,sizeof(char)) ;

   sprintf(name, "cheat_%d", index);

   //Break the code into Parts
   for (cursor=0;;cursor++)
   {
      if (ISHEXDEC)
         codeLine[codePos++] = toupper(code[cursor]) ;
      else
      {
         if ( codePos >= 12 )
         {
            if ( codePos == 12 )
            {
               for ( i = 0 ; i < 4 ; i++ )
                  codeLine[codePos-i] = codeLine[(codePos-i)-1] ;
               codeLine[8] = ' ' ;
               codeLine[13] = '\0' ;
               cheatsAddCBACode(codeLine, name);
            } else if ( codePos == 16 )
            {
               codeLine[16] = '\0' ;
               cheatsAddGSACode(codeLine, name, 1);
            } else 
            {
               codeLine[codePos] = '\0' ;
            }
            codePos = 0 ;
            memset(codeLine,0,codeLineSize) ;
         }
      }
      if (!code[cursor])
         break;
   }

	free(codeLine);
}


static unsigned g_audio_frames;
static unsigned g_video_frames;

void systemOnWriteDataToSoundBuffer(int16_t *finalWave, int length)
{
   Audio_Write(finalWave, (length) >> 1);
   g_audio_frames += frames;
}


void systemDrawScreen()
{
	Update_Video_Ingame((uint16_t*)pix);
	g_video_frames++;
	has_frame = 1;
}

void systemMessage(const char* fmt, ...)
{
   char buffer[256];
   va_list ap;
   va_start(ap, fmt);
   vsprintf(buffer, fmt, ap);
   printf("%s\n", buffer);
   va_end(ap);
}

void EEPROM_file(char* tmp, uint_fast8_t load)
{
	FILE* fp;

	if (flashSize < 1) return;

	switch(load)
	{
		case 0:
			fp = fopen(tmp, "wb");
			if (!fp) return;
			fwrite(libretro_save_buf, sizeof(uint8_t), libretro_save_size, fp);
			fclose(fp);
		break;
		case 1:
			fp = fopen(tmp, "rb");
			if (!fp) return;
			fread(libretro_save_buf, sizeof(uint8_t), libretro_save_size, fp);
			fclose(fp);
		break;
	}
}

void SaveState(char* tmp, uint_fast8_t load)
{
	FILE* fp;
	uint8_t* data;

	switch(load)
	{
		case 0:
			fp = fopen(tmp, "wb");
			if (!fp) return;
			data = (uint8_t*) malloc(serialize_size);
			CPUWriteState((uint8_t*)data, serialize_size);
			fwrite(data, sizeof(uint8_t), serialize_size, fp);
			if (data) free(data);
			if (fp) fclose(fp);
		break;
		case 1:
			fp = fopen(tmp, "rb");
			if (!fp) return;
			data = (uint8_t*) malloc(serialize_size);
			fread(data, sizeof(uint8_t), serialize_size, fp);
			CPUReadState((uint8_t*)data, serialize_size);
			if (data) free(data);
			if (fp) fclose(fp);
		break;
	}
}

int main(int argc, char* argv[])
{
	uint_fast8_t ret;
	
    printf("Starting VBA-Next\n");
    
    if (argc < 2)
	{
		printf("Specify a ROM to load in memory\n");
		return 0;
	}

	snprintf(GameName_emu, sizeof(GameName_emu), "%s", basename(argv[1]));
	Init_Video();
	
#if USE_FRAME_SKIP
	SetFrameskip(get_frameskip_code());
#endif
	ret = CPULoadRom(argv[1]);
	if (ret < 1)
	{
		printf("Could not load ROM in memory\n");
		return 0;
	}
	gba_init();
	
	vbanext_init();
	/* Init_Configuration also takes care of EEPROM saves so execute it after the game has been loaded in memory. */
	Init_Configuration();
	
	Audio_Init();
	
	while(!done)
	{
		switch(emulator_state)
		{
			case 0:
				vbanext_run();
			break;
			case 1:
				Menu();
			break;
		}
	}
	
	vbanext_deinit();
    Audio_Close();
	Close_Video();
}
