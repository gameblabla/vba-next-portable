#include <stdio.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <tinyalsa/pcm.h>

#include "sound_output.h"
struct pcm *pcm_out;

#define CARD_DEFAULT 0

uint32_t Audio_Init()
{
#ifdef BLOCKING_AUDIO
	int flags = PCM_OUT;
#else
	int flags = PCM_OUT | PCM_NONBLOCK;
#endif
    struct pcm_config config = {
        .channels = 2,
        .rate = SOUND_OUTPUT_FREQUENCY,
        .format = PCM_FORMAT_S16_LE
    };
    
    config.period_size = SOUND_SAMPLES_SIZE;
    config.period_count = 2;
    config.start_threshold = config.period_size;
    config.silence_threshold = config.period_size * 2;
    config.stop_threshold = config.period_size * 2;
    
    pcm_out = pcm_open(CARD_DEFAULT, 0, flags, &config);
    if (!pcm_out)
    {
		printf("Does not exit\n");
		return 0;
	}
	
	return 1;
}

void Audio_Write(int16_t* buffer, uint32_t buffer_size)
{
	pcm_writei(pcm_out, buffer, buffer_size);
}

void Audio_Close()
{
	if (pcm_out) pcm_close(pcm_out);
}
