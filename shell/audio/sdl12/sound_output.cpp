#include <stdio.h>
#include <stdint.h>
#include <SDL/SDL.h>
#include "sound_output.h"

/* Most of the sound code was taken from FBA-SDL by DmitrySmagin so many thanks to him ! */
static SDL_mutex *sound_mutex;
static SDL_cond *sound_cv;

/* Using Mutexes by default but allowing disabling them on compilation */
#ifdef SDLAUDIO_NOMUTEXES
#define mutex 0
#else
#define mutex 1
#endif

static int32_t BUFFSIZE;
static uint8_t *buffer;
static int32_t buf_read_pos = 0;
static uint32_t buf_write_pos = 0;
static int32_t buffered_bytes = 0;

static int32_t sdl_write_buffer_m(uint8_t* data, int32_t len)
{
	SDL_LockMutex(sound_mutex);
	for(uint8_t i = 0; i < len; i += 4) 
	{
		while(buffered_bytes == BUFFSIZE) SDL_CondWait(sound_cv, sound_mutex);

		*(int32_t*)((char*)(buffer + buf_write_pos)) = *(int32_t*)((char*)(data + i));
		//memcpy(buffer + buf_write_pos, data + i, 4);
		buf_write_pos = (buf_write_pos + 4) % BUFFSIZE;
		buffered_bytes += 4;
	}
	SDL_CondSignal(sound_cv);
	SDL_UnlockMutex(sound_mutex);
	return len;
}

static void sdl_write_buffer(uint8_t* data, int32_t len)
{
	for(int32_t i = 0; i < len; i += 4) 
	{
		if(buffered_bytes == BUFFSIZE) return; // just drop samples
		*(int32_t*)((char*)(buffer + buf_write_pos)) = *(int32_t*)((char*)(data + i));
		//memcpy(buffer + buf_write_pos, data + i, 4);
		buf_write_pos = (buf_write_pos + 4) % BUFFSIZE;
		buffered_bytes += 4;
	}
}

static int32_t sdl_read_buffer(uint8_t* data, int32_t len)
{
	if (buffered_bytes >= len) 
	{
		if(buf_read_pos + len <= BUFFSIZE ) 
		{
			memcpy(data, buffer + buf_read_pos, len);
		} 
		else 
		{
			int32_t tail = BUFFSIZE - buf_read_pos;
			memcpy(data, buffer + buf_read_pos, tail);
			memcpy(data + tail, buffer, len - tail);
		}
		buf_read_pos = (buf_read_pos + len) % BUFFSIZE;
		buffered_bytes -= len;
	}

	return len;
}

void sdl_callback_m(void *unused, uint8_t *stream, int32_t len)
{
	SDL_LockMutex(sound_mutex);

	sdl_read_buffer((uint8_t *)stream, len);

	SDL_CondSignal(sound_cv);
	SDL_UnlockMutex(sound_mutex);
}

void sdl_callback(void *unused, uint8_t *stream, int32_t len)
{
	sdl_read_buffer((uint8_t *)stream, len);
}

uint32_t Audio_Init()
{
	SDL_AudioSpec aspec, obtained;
	
	BUFFSIZE = SOUND_SAMPLES_SIZE * 2 * 2 * 8;
	buffer = (uint8_t *) malloc(BUFFSIZE);

	/* Add some silence to the buffer */
	buffered_bytes = 0;
	buf_read_pos = 0;
	buf_write_pos = 0;

	aspec.format   = AUDIO_S16SYS;
	aspec.freq     = SOUND_OUTPUT_FREQUENCY;
	aspec.channels = 2;
	aspec.samples  = SOUND_SAMPLES_SIZE;
	aspec.callback = (mutex ? sdl_callback_m : sdl_callback);
	aspec.userdata = NULL;

	/* initialize the SDL Audio system */
	if (SDL_InitSubSystem (SDL_INIT_AUDIO)) 
	{
		printf("SDL: Initializing of SDL Audio failed: %s.\n", SDL_GetError());
		return 1;
	}

	/* Open the audio device and start playing sound! */
	if(SDL_OpenAudio(&aspec, &obtained) < 0) 
	{
		printf("SDL: Unable to open audio: %s\n", SDL_GetError());
		return 1;
	}

	if (mutex) 
	{
		sound_mutex = SDL_CreateMutex();
		sound_cv = SDL_CreateCond();
		printf("Using mutexes for sync\n");
	}
	
	SDL_PauseAudio(0);
	
	return 0;
}

void Audio_Write(int16_t* buffer, uint32_t buffer_size)
{
	SDL_LockAudio();
	sdl_write_buffer((uint8_t*)buffer, buffer_size * 4);
	SDL_UnlockAudio();
}

void Audio_Close()
{
	SDL_PauseAudio(1);
	if (buffer)
	{
		free(buffer);
		buffer = NULL;
	}
	
	if (mutex) 
	{
		SDL_LockMutex(sound_mutex);
		buffered_bytes = BUFFSIZE;
		SDL_CondSignal(sound_cv);
		SDL_UnlockMutex(sound_mutex);
		SDL_Delay(100);
		SDL_DestroyCond(sound_cv);
		SDL_DestroyMutex(sound_mutex);
	}
	SDL_CloseAudio();
	if (SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO) SDL_QuitSubSystem(SDL_INIT_AUDIO);
}
