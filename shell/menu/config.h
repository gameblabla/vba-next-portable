#ifndef CONFIG_H__
#define CONFIG_H__

#define HORIZONTAL_CONTROLS 0
#define VERTICAL_CONTROLS 1

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int32_t fullscreen;
	/* For input remapping */
	uint32_t config_buttons[19];
	int32_t frameskip;
} t_config;
extern t_config option;

#ifdef __cplusplus
};
#endif

#endif
