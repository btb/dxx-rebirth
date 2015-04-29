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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Lighting system prototypes, structures, etc.
 *
 */

#pragma once

#include "maths.h"

#ifdef __cplusplus
#include "fwdsegment.h"
#include "fwdvalptridx.h"

struct g3s_lrgb;

#define MAX_LIGHT       0x10000     // max value

#define MIN_LIGHT_DIST  (F1_0*4)

extern array<g3s_lrgb, MAX_VERTICES> Dynamic_light;
extern object *old_viewer;

// compute the average dynamic light in a segment.  Takes the segment number
g3s_lrgb compute_seg_dynamic_light(segnum_t segnum);

// compute the lighting for an object.  Takes a pointer to the object,
// and possibly a rotated 3d point.  If the point isn't specified, the
// object's center point is rotated.
g3s_lrgb compute_object_light(vobjptridx_t obj,const vms_vector *rotated_pnt);

// turn headlight boost on & off
void toggle_headlight_active(void);
void start_lighting_frame(objptr_t viewer);

#endif
