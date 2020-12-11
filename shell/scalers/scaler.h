#ifndef SCALER_H
#define SCALER_H

#include <stdint.h>

/* Generic */
extern void bitmap_scale(uint32_t startx, uint32_t starty, uint32_t viswidth, uint32_t visheight, uint32_t newwidth, uint32_t newheight,uint32_t pitchsrc,uint32_t pitchdest, uint16_t __restrict__ * src, uint16_t __restrict__ * dst);
extern void gba_upscale(uint16_t __restrict__ *to, uint16_t __restrict__ *from, uint32_t src_x, uint32_t src_y, uint32_t src_pitch, uint32_t dst_pitch);
extern void gba_upscale_aspect(uint16_t __restrict__ *to, uint16_t __restrict__ *from, uint32_t src_x, uint32_t src_y, uint32_t src_pitch, uint32_t dst_pitch);
extern void upscale_160x240_to_240x240_bilinearish(uint16_t __restrict__* Src16, uint16_t __restrict__ *  Dst16);

#endif
