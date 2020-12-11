#include <stdio.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <tinyalsa/pcm.h>

#include "sound_output.h"
struct pcm *pcm_out;

uint32_t Audio_Init()
{
	struct pcm_config config;
	
    config.channels = 2;
    config.rate = SOUND_OUTPUT_FREQUENCY;
    config.period_size = SOUND_SAMPLES_SIZE;
    config.period_count = SOUND_SAMPLES_SIZE / 512;
	config.format = PCM_FORMAT_S16_LE;
	
    pcm_out = pcm_open(0, 0, PCM_OUT, &config);
    if (!pcm_out)
    {
        printf("failed to allocate memory for PCM\n");
        return -1;
    }
    else if (!pcm_is_ready(pcm_out))
    {
        pcm_close(pcm_out);
        printf("failed to open PCM\n");
        return -1;
    }
    
	return 1;
}

void Audio_Write(int16_t* buffer, uint32_t buffer_size)
{
	if (pcm_out != NULL) return;
	pcm_writei(pcm_out, buffer, buffer_size);
}

void Audio_Close()
{
	if (pcm_out) pcm_close(pcm_out);
}
