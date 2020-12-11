#include <cstddef>
#include "scaler.h"

#define AVERAGE(z, x) ((((z) & 0xF7DEF7DE) >> 1) + (((x) & 0xF7DEF7DE) >> 1))
#define AVERAGEHI(AB) ((((AB) & 0xF7DE0000) >> 1) + (((AB) & 0xF7DE) << 15))
#define AVERAGELO(CD) ((((CD) & 0xF7DE) >> 1) + (((CD) & 0xF7DE0000) >> 17))

// Support math
#define Half(A) (((A) >> 1) & 0x7BEF)
#define Quarter(A) (((A) >> 2) & 0x39E7)
// Error correction expressions to piece back the lower bits together
#define RestHalf(A) ((A) & 0x0821)
#define RestQuarter(A) ((A) & 0x1863)

// Error correction expressions for quarters of pixels
#define Corr1_3(A, B)     Quarter(RestQuarter(A) + (RestHalf(B) << 1) + RestQuarter(B))
#define Corr3_1(A, B)     Quarter((RestHalf(A) << 1) + RestQuarter(A) + RestQuarter(B))

// Error correction expressions for halves
#define Corr1_1(A, B)     ((A) & (B) & 0x0821)

// Quarters
#define Weight1_3(A, B)   (Quarter(A) + Half(B) + Quarter(B) + Corr1_3(A, B))
#define Weight3_1(A, B)   (Half(A) + Quarter(A) + Quarter(B) + Corr3_1(A, B))



#define Weight1_1(A, B)   (Half(A) + Half(B) + Corr1_1(A, B))

void upscale_160x240_to_240x240_bilinearish(uint16_t __restrict__* Src16, uint16_t __restrict__ *  Dst16)
{
	// There are 80 blocks of 2 pixels vertically, and 240 of 1 horizontally.
	// Each block of 2x1 becomes 3x1.
	uint32_t BlockX, BlockY;
	uint16_t* BlockSrc;
	uint16_t* BlockDst;
	for (BlockY = 0; BlockY < 80; BlockY++)
	{
		BlockSrc = Src16 + BlockY * 240 * 2;
		BlockDst = Dst16 + BlockY * 240 * 3;
		for (BlockX = 0; BlockX < 240; BlockX++)
		{
			/* Vertically:
			 * Before(2):
			 * (a)(b)
			 * After(3):
			 * (a)(ab)(b)
			 */

			// -- Column 1 --
			uint16_t  _1 = *(BlockSrc               );
			*(BlockDst               ) = _1;
			uint16_t  _2 = *(BlockSrc            + 240*1);
			*(BlockDst            + 240*1) = Weight1_1( _1,  _2);
			*(BlockDst            + 240*2) = _2;

			BlockSrc += 1;
			BlockDst += 1;
		}
	}
}

/* alekmaul's scaler taken from mame4all */
void bitmap_scale(uint32_t startx, uint32_t starty, uint32_t viswidth, uint32_t visheight, uint32_t newwidth, uint32_t newheight,uint32_t pitchsrc,uint32_t pitchdest, uint16_t __restrict__* src, uint16_t __restrict__ *  dst)
{
    uint32_t W,H,ix,iy,x,y;
    x=startx<<16;
    y=starty<<16;
    W=newwidth;
    H=newheight;
    ix=(viswidth<<16)/W;
    iy=(visheight<<16)/H;

    do 
    {
        uint16_t* buffer_mem=&src[(y>>16)*pitchsrc];
        W=newwidth; x=startx<<16;
        do 
        {
            *dst++=buffer_mem[x>>16];
            x+=ix;
        } while (--W);
        dst+=pitchdest;
        y+=iy;
    } while (--H);
}



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


void gba_upscale(uint16_t __restrict__ *to, uint16_t __restrict__ *from, uint32_t src_x, uint32_t src_y, uint32_t src_pitch, uint32_t dst_pitch)
{
	/* Before:
	 *    a b c d e f
	 *    g h i j k l
	 *
	 * After (multiple letters = average):
	 *    a    ab   bc   c    d    de   ef   f
	 *    ag   abgh bchi ci   dj   dejk efkl fl
	 *    g    gh   hi   i    j    jk   kl   l
	 */

	const uint32_t dst_x = src_x * 4 / 3;
	const uint32_t src_skip = src_pitch - src_x * sizeof(uint16_t),
	               dst_skip = dst_pitch - dst_x * sizeof(uint16_t);

	uint32_t x, y;

	for (y = 0; y < src_y; y += 2) {
		for (x = 0; x < src_x / 6; x++) {
			// -- Row 1 --
			// Read RGB565 elements in the source grid.
			// The notation is high_low (little-endian).
			uint32_t b_a = (*(uint32_t*) (from    )),
			         d_c = (*(uint32_t*) (from + 2)),
			         f_e = (*(uint32_t*) (from + 4));

			// Generate ab_a from b_a.
			*(uint32_t*) (to) = likely(Hi(b_a) == Lo(b_a))
				? b_a
				: Lo(b_a) /* 'a' verbatim to low pixel */ |
				  Raise(Average(Hi(b_a), Lo(b_a))) /* ba to high pixel */;

			// Generate c_bc from b_a and d_c.
			*(uint32_t*) (to + 2) = likely(Hi(b_a) == Lo(d_c))
				? Lo(d_c) | Raise(Lo(d_c))
				: Raise(Lo(d_c)) /* 'c' verbatim to high pixel */ |
				  Average(Lo(d_c), Hi(b_a)) /* bc to low pixel */;

			// Generate de_d from d_c and f_e.
			*(uint32_t*) (to + 4) = likely(Hi(d_c) == Lo(f_e))
				? Lo(f_e) | Raise(Lo(f_e))
				: Hi(d_c) /* 'd' verbatim to low pixel */ |
				  Raise(Average(Lo(f_e), Hi(d_c))) /* de to high pixel */;

			// Generate f_ef from f_e.
			*(uint32_t*) (to + 6) = likely(Hi(f_e) == Lo(f_e))
				? f_e
				: Raise(Hi(f_e)) /* 'f' verbatim to high pixel */ |
				  Average(Hi(f_e), Lo(f_e)) /* ef to low pixel */;

			if (likely(y + 1 < src_y))  // Is there a source row 2?
			{
				// -- Row 2 --
				uint32_t h_g = (*(uint32_t*) ((uint8_t*) from + src_pitch    )),
				         j_i = (*(uint32_t*) ((uint8_t*) from + src_pitch + 4)),
				         l_k = (*(uint32_t*) ((uint8_t*) from + src_pitch + 8));

				// Generate abgh_ag from b_a and h_g.
				uint32_t bh_ag = Average32(b_a, h_g);
				*(uint32_t*) ((uint8_t*) to + dst_pitch) = likely(Hi(bh_ag) == Lo(bh_ag))
					? bh_ag
					: Lo(bh_ag) /* ag verbatim to low pixel */ |
					  Raise(Average(Hi(bh_ag), Lo(bh_ag))) /* abgh to high pixel */;

				// Generate ci_bchi from b_a, d_c, h_g and j_i.
				uint32_t ci_bh =
					Hi(bh_ag) /* bh verbatim to low pixel */ |
					Raise(Average(Lo(d_c), Lo(j_i))) /* ci to high pixel */;
				*(uint32_t*) ((uint8_t*) to + dst_pitch + 4) = likely(Hi(ci_bh) == Lo(ci_bh))
					? ci_bh
					: Raise(Hi(ci_bh)) /* ci verbatim to high pixel */ |
					  Average(Hi(ci_bh), Lo(ci_bh)) /* bchi to low pixel */;

				// Generate fl_efkl from f_e and l_k.
				uint32_t fl_ek = Average32(f_e, l_k);
				*(uint32_t*) ((uint8_t*) to + dst_pitch + 12) = likely(Hi(fl_ek) == Lo(fl_ek))
					? fl_ek
					: Raise(Hi(fl_ek)) /* fl verbatim to high pixel */ |
					  Average(Hi(fl_ek), Lo(fl_ek)) /* efkl to low pixel */;

				// Generate dejk_dj from d_c, f_e, j_i and l_k.
				uint32_t ek_dj =
					Raise(Lo(fl_ek)) /* ek verbatim to high pixel */ |
					Average(Hi(d_c), Hi(j_i)) /* dj to low pixel */;
				*(uint32_t*) ((uint8_t*) to + dst_pitch + 8) = likely(Hi(ek_dj) == Lo(ek_dj))
					? ek_dj
					: Lo(ek_dj) /* dj verbatim to low pixel */ |
					  Raise(Average(Hi(ek_dj), Lo(ek_dj))) /* dejk to high pixel */;

				// -- Row 3 --
				// Generate gh_g from h_g.
				*(uint32_t*) ((uint8_t*) to + dst_pitch * 2) = likely(Hi(h_g) == Lo(h_g))
					? h_g
					: Lo(h_g) /* 'g' verbatim to low pixel */ |
					  Raise(Average(Hi(h_g), Lo(h_g))) /* gh to high pixel */;

				// Generate i_hi from g_h and j_i.
				*(uint32_t*) ((uint8_t*) to + dst_pitch * 2 + 4) = likely(Hi(h_g) == Lo(j_i))
					? Lo(j_i) | Raise(Lo(j_i))
					: Raise(Lo(j_i)) /* 'i' verbatim to high pixel */ |
					  Average(Lo(j_i), Hi(h_g)) /* hi to low pixel */;

				// Generate jk_j from j_i and l_k.
				*(uint32_t*) ((uint8_t*) to + dst_pitch * 2 + 8) = likely(Hi(j_i) == Lo(l_k))
					? Lo(l_k) | Raise(Lo(l_k))
					: Hi(j_i) /* 'j' verbatim to low pixel */ |
					  Raise(Average(Hi(j_i), Lo(l_k))) /* jk to high pixel */;

				// Generate l_kl from l_k.
				*(uint32_t*) ((uint8_t*) to + dst_pitch * 2 + 12) = likely(Hi(l_k) == Lo(l_k))
					? l_k
					: Raise(Hi(l_k)) /* 'l' verbatim to high pixel */ |
					  Average(Hi(l_k), Lo(l_k)) /* kl to low pixel */;
			}

			from += 6;
			to += 8;
		}

		// Skip past the waste at the end of the first line, if any,
		// then past 1 whole lines of source and 2 of destination.
		from = (uint16_t*) ((uint8_t*) from + src_skip +     src_pitch);
		to   = (uint16_t*) ((uint8_t*) to   + dst_skip + 2 * dst_pitch);
	}
}

void gba_upscale_aspect(uint16_t __restrict__ *to, uint16_t __restrict__ *from, uint32_t src_x, uint32_t src_y, uint32_t src_pitch, uint32_t dst_pitch)
{
	/* Before:
	 *    a b c d e f
	 *    g h i j k l
	 *    m n o p q r
	 *
	 * After (multiple letters = average):
	 *    a    ab   bc   c    d    de   ef   f
	 *    ag   abgh bchi ci   dj   dejk efkl fl
	 *    gm   ghmn hino io   jp   jkpq klqr lr
	 *    m    mn   no   o    p    pq   qr   r
	 */

	const uint32_t dst_x = src_x * 4 / 3;
	const uint32_t src_skip = src_pitch - src_x * sizeof(uint16_t),
	               dst_skip = dst_pitch - dst_x * sizeof(uint16_t);

	uint32_t x, y;

	for (y = 0; y < src_y; y += 3) {
		for (x = 0; x < src_x / 6; x++) {
			// -- Row 1 --
			// Read RGB565 elements in the source grid.
			// The notation is high_low (little-endian).
			uint32_t b_a = (*(uint32_t*) (from    )),
			         d_c = (*(uint32_t*) (from + 2)),
			         f_e = (*(uint32_t*) (from + 4));

			// Generate ab_a from b_a.
			*(uint32_t*) (to) = likely(Hi(b_a) == Lo(b_a))
				? b_a
				: Lo(b_a) /* 'a' verbatim to low pixel */ |
				  Raise(Average(Hi(b_a), Lo(b_a))) /* ba to high pixel */;

			// Generate c_bc from b_a and d_c.
			*(uint32_t*) (to + 2) = likely(Hi(b_a) == Lo(d_c))
				? Lo(d_c) | Raise(Lo(d_c))
				: Raise(Lo(d_c)) /* 'c' verbatim to high pixel */ |
				  Average(Lo(d_c), Hi(b_a)) /* bc to low pixel */;

			// Generate de_d from d_c and f_e.
			*(uint32_t*) (to + 4) = likely(Hi(d_c) == Lo(f_e))
				? Lo(f_e) | Raise(Lo(f_e))
				: Hi(d_c) /* 'd' verbatim to low pixel */ |
				  Raise(Average(Lo(f_e), Hi(d_c))) /* de to high pixel */;

			// Generate f_ef from f_e.
			*(uint32_t*) (to + 6) = likely(Hi(f_e) == Lo(f_e))
				? f_e
				: Raise(Hi(f_e)) /* 'f' verbatim to high pixel */ |
				  Average(Hi(f_e), Lo(f_e)) /* ef to low pixel */;

			if (likely(y + 1 < src_y))  // Is there a source row 2?
			{
				// -- Row 2 --
				uint32_t h_g = (*(uint32_t*) ((uint8_t*) from + src_pitch    )),
				         j_i = (*(uint32_t*) ((uint8_t*) from + src_pitch + 4)),
				         l_k = (*(uint32_t*) ((uint8_t*) from + src_pitch + 8));

				// Generate abgh_ag from b_a and h_g.
				uint32_t bh_ag = Average32(b_a, h_g);
				*(uint32_t*) ((uint8_t*) to + dst_pitch) = likely(Hi(bh_ag) == Lo(bh_ag))
					? bh_ag
					: Lo(bh_ag) /* ag verbatim to low pixel */ |
					  Raise(Average(Hi(bh_ag), Lo(bh_ag))) /* abgh to high pixel */;

				// Generate ci_bchi from b_a, d_c, h_g and j_i.
				uint32_t ci_bh =
					Hi(bh_ag) /* bh verbatim to low pixel */ |
					Raise(Average(Lo(d_c), Lo(j_i))) /* ci to high pixel */;
				*(uint32_t*) ((uint8_t*) to + dst_pitch + 4) = likely(Hi(ci_bh) == Lo(ci_bh))
					? ci_bh
					: Raise(Hi(ci_bh)) /* ci verbatim to high pixel */ |
					  Average(Hi(ci_bh), Lo(ci_bh)) /* bchi to low pixel */;

				// Generate fl_efkl from f_e and l_k.
				uint32_t fl_ek = Average32(f_e, l_k);
				*(uint32_t*) ((uint8_t*) to + dst_pitch + 12) = likely(Hi(fl_ek) == Lo(fl_ek))
					? fl_ek
					: Raise(Hi(fl_ek)) /* fl verbatim to high pixel */ |
					  Average(Hi(fl_ek), Lo(fl_ek)) /* efkl to low pixel */;

				// Generate dejk_dj from d_c, f_e, j_i and l_k.
				uint32_t ek_dj =
					Raise(Lo(fl_ek)) /* ek verbatim to high pixel */ |
					Average(Hi(d_c), Hi(j_i)) /* dj to low pixel */;
				*(uint32_t*) ((uint8_t*) to + dst_pitch + 8) = likely(Hi(ek_dj) == Lo(ek_dj))
					? ek_dj
					: Lo(ek_dj) /* dj verbatim to low pixel */ |
					  Raise(Average(Hi(ek_dj), Lo(ek_dj))) /* dejk to high pixel */;

				if (likely(y + 2 < src_y))  // Is there a source row 3?
				{
					// -- Row 3 --
					uint32_t n_m = (*(uint32_t*) ((uint8_t*) from + src_pitch * 2    )),
					         p_o = (*(uint32_t*) ((uint8_t*) from + src_pitch * 2 + 4)),
					         r_q = (*(uint32_t*) ((uint8_t*) from + src_pitch * 2 + 8));

					// Generate ghmn_gm from h_g and n_m.
					uint32_t hn_gm = Average32(h_g, n_m);
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 2) = likely(Hi(hn_gm) == Lo(hn_gm))
						? hn_gm
						: Lo(hn_gm) /* gm verbatim to low pixel */ |
						  Raise(Average(Hi(hn_gm), Lo(hn_gm))) /* ghmn to high pixel */;

					// Generate io_hino from h_g, j_i, n_m and p_o.
					uint32_t io_hn =
						Hi(hn_gm) /* hn verbatim to low pixel */ |
						Raise(Average(Lo(j_i), Lo(p_o))) /* io to high pixel */;
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 2 + 4) = likely(Hi(io_hn) == Lo(io_hn))
						? io_hn
						: Raise(Hi(io_hn)) /* io verbatim to high pixel */ |
						  Average(Hi(io_hn), Lo(io_hn)) /* hino to low pixel */;

					// Generate lr_klqr from l_k and r_q.
					uint32_t lr_kq = Average32(l_k, r_q);
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 2 + 12) = likely(Hi(lr_kq) == Lo(lr_kq))
						? lr_kq
						: Raise(Hi(lr_kq)) /* lr verbatim to high pixel */ |
						  Average(Hi(lr_kq), Lo(lr_kq)) /* klqr to low pixel */;

					// Generate jkpq_jp from j_i, l_k, p_o and r_q.
					uint32_t kq_jp =
						Raise(Lo(lr_kq)) /* kq verbatim to high pixel */ |
						Average(Hi(j_i), Hi(p_o)) /* jp to low pixel */;
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 2 + 8) = likely(Hi(kq_jp) == Lo(kq_jp))
						? kq_jp
						: Lo(kq_jp) /* jp verbatim to low pixel */ |
						  Raise(Average(Hi(kq_jp), Lo(kq_jp))) /* jkpq to high pixel */;

					// -- Row 4 --
					// Generate mn_m from n_m.
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 3) = likely(Hi(n_m) == Lo(n_m))
						? n_m
						: Lo(n_m) /* 'm' verbatim to low pixel */ |
						  Raise(Average(Hi(n_m), Lo(n_m))) /* mn to high pixel */;

					// Generate o_no from n_m and p_o.
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 3 + 4) = likely(Hi(n_m) == Lo(p_o))
						? Lo(p_o) | Raise(Lo(p_o))
						: Raise(Lo(p_o)) /* 'o' verbatim to high pixel */ |
						  Average(Lo(p_o), Hi(n_m)) /* no to low pixel */;

					// Generate pq_p from p_o and r_q.
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 3 + 8) = likely(Hi(p_o) == Lo(r_q))
						? Lo(r_q) | Raise(Lo(r_q))
						: Hi(p_o) /* 'p' verbatim to low pixel */ |
						  Raise(Average(Hi(p_o), Lo(r_q))) /* pq to high pixel */;

					// Generate r_qr from r_q.
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 3 + 12) = likely(Hi(r_q) == Lo(r_q))
						? r_q
						: Raise(Hi(r_q)) /* 'r' verbatim to high pixel */ |
						  Average(Hi(r_q), Lo(r_q)) /* qr to low pixel */;
				}
			}

			from += 6;
			to += 8;
		}

		// Skip past the waste at the end of the first line, if any,
		// then past 2 whole lines of source and 3 of destination.
		from = (uint16_t*) ((uint8_t*) from + src_skip + 2 * src_pitch);
		to   = (uint16_t*) ((uint8_t*) to   + dst_skip + 3 * dst_pitch);
	}
}
