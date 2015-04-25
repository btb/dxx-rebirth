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
 * Headers for physics functions and data
 *
 */


#ifndef _PHYSICS_H
#define _PHYSICS_H

#include "vecmat.h"
#include "fvi.h"

#ifdef __cplusplus

//#define FL_NORMAL  0
//#define FL_TURBO   1
//#define FL_HOVER   2
//#define FL_REVERSE 3

// these global vars are set after a call to do_physics_sim().  Ugly, I know.
// list of segments went through
extern unsigned n_phys_segs;
extern array<segnum_t, MAX_FVI_SEGS> phys_seglist;

// Simulate a physics object for this frame
void do_physics_sim(vobjptridx_t obj);

// Applies an instantaneous force on an object, resulting in an instantaneous
// change in velocity.
void phys_apply_force(vobjptr_t obj, const vms_vector &force_vec);
void phys_apply_rot(vobjptr_t obj, const vms_vector &force_vec);

// this routine will set the thrust for an object to a value that will
// (hopefully) maintain the object's current velocity
void set_thrust_from_velocity(vobjptr_t obj);
void check_and_fix_matrix(vms_matrix &m);
void physics_turn_towards_vector(const vms_vector &goal_vector, vobjptr_t obj, fix rate);

#endif

#endif /* _PHYSICS_H */
