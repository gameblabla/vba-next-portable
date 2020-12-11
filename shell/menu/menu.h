#ifndef MENU_H
#define MENU_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PATH_MAX
#define PATH_MAX 2048
#endif

#define RGB565(r,g,b) ((r << 8) | (g << 3) | (b >> 3))

extern uint32_t emulator_state;
extern uint32_t done;

extern void Menu();
extern void Init_Configuration();
extern void EEPROM_Menu(uint_fast8_t load_mode);

#ifdef __cplusplus
};
#endif


#endif
