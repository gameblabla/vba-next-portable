#ifndef SCALER_H
#define SCALER_H

#include <stdint.h>

#define prefetch(a,b)   __builtin_prefetch(a,b)
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#define Average(A, B) ((((A) & 0xF7DE) >> 1) + (((B) & 0xF7DE) >> 1) + ((A) & (B) & 0x0821))

/* Calculates the average of two pairs of RGB565 pixels. The result is, in
 * the lower bits, the average of both lower pixels, and in the upper bits,
 * the average of both upper pixels. */
#define Average32(A, B) ((((A) & 0xF7DEF7DE) >> 1) + (((B) & 0xF7DEF7DE) >> 1) + ((A) & (B) & 0x08210821))

/* Raises a pixel from the lower half to the upper half of a pair. */
#define Raise(N) ((N) << 16)

/* Extracts the upper pixel of a pair into the lower pixel of a pair. */
#define Hi(N) ((N) >> 16)

/* Extracts the lower pixel of a pair. */
#define Lo(N) ((N) & 0xFFFF)

/* Calculates the average of two RGB565 pixels. The source of the pixels is
 * the lower 16 bits of both parameters. The result is in the lower 16 bits.
 * The average is weighted so that the first pixel contributes 3/4 of its
 * color and the second pixel contributes 1/4. */
#define AverageQuarters3_1(A, B) ( (((A) & 0xF7DE) >> 1) + (((A) & 0xE79C) >> 2) + (((B) & 0xE79C) >> 2) + ((( (( ((A) & 0x1863) + ((A) & 0x0821) ) << 1) + ((B) & 0x1863) ) >> 2) & 0x1863) )

/* Generic */
extern void bitmap_scale(uint32_t startx, uint32_t starty, uint32_t viswidth, uint32_t visheight, uint32_t newwidth, uint32_t newheight,uint32_t pitchsrc,uint32_t pitchdest, uint16_t __restrict__ * src, uint16_t __restrict__ * dst);
extern void gba_upscale(uint16_t __restrict__ *to, uint16_t __restrict__ *from, uint32_t src_x, uint32_t src_y, uint32_t src_pitch, uint32_t dst_pitch);
extern void gba_upscale_aspect(uint16_t __restrict__ *to, uint16_t __restrict__ *from, uint32_t src_x, uint32_t src_y, uint32_t src_pitch, uint32_t dst_pitch);

#endif
