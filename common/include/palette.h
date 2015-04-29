/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Protoypes for palette functions
 *
 */


#ifndef _PALETTE_H
#define _PALETTE_H

#include "pstypes.h"

#ifdef __cplusplus
#include <cstdint>
#include "dxxsconf.h"
#include "compiler-array.h"

struct rgb_t {
	ubyte r,g,b;
};

typedef uint8_t color_t;

static inline bool operator==(const rgb_t &a, const rgb_t &b) { return a.r == b.r && a.g == b.g && a.b == b.b; }

struct palette_array_t : public array<rgb_t, 256> {};

#ifdef DXX_BUILD_DESCENT_II
#define DEFAULT_LEVEL_PALETTE "groupa.256" //don't confuse with D2_DEFAULT_PALETTE
#endif

void copy_bound_palette(palette_array_t &d, const palette_array_t &s);
void copy_diminish_palette(palette_array_t &palette, const ubyte *p);
void diminish_palette(palette_array_t &palette);
extern void gr_palette_set_gamma( int gamma );
extern int gr_palette_get_gamma();
void gr_palette_load( palette_array_t &pal );
color_t gr_find_closest_color_current( int r, int g, int b );
extern void gr_palette_read(palette_array_t &palette);
extern void init_computed_colors(void);
extern ubyte gr_palette_gamma;
extern palette_array_t gr_current_pal;

#endif

#endif
