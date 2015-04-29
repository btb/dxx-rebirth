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
 * Header file for stuff moved from segment.c to gameseg.c.
 *
 */


#ifndef _GAMESEG_H
#define _GAMESEG_H

#include "pstypes.h"
#include "maths.h"
#include "vecmat.h"
#include "segment.h"

#ifdef __cplusplus
#include <utility>
#include "dxxsconf.h"
#include "compiler-array.h"

struct segmasks
{
   short facemask;     //which faces sphere pokes through (12 bits)
   sbyte sidemask;     //which sides sphere pokes through (6 bits)
   sbyte centermask;   //which sides center point is on back of (6 bits)
};

struct segment_depth_array_t : public array<ubyte, MAX_SEGMENTS> {};

extern unsigned Highest_vertex_index;                   // Highest index in Vertices and Vertex_active, an efficiency hack
extern int	Doing_lighting_hack_flag;

void compute_center_point_on_side(vms_vector &vp,vcsegptr_t sp,int side);
static inline vms_vector compute_center_point_on_side(const vcsegptr_t sp,int side)
{
	vms_vector v;
	return compute_center_point_on_side(v, sp, side), v;
}
void compute_segment_center(vms_vector &vp,vcsegptr_t sp);
static inline vms_vector compute_segment_center(vcsegptr_t sp)
{
	vms_vector v;
	compute_segment_center(v, sp);
	return v;
}
int_fast32_t find_connect_side(vcsegptridx_t base_seg, vcsegptr_t con_seg) __attribute_warn_unused_result;

struct side_vertnum_list_t : array<int, 4> {};

// Fill in array with four absolute point numbers for a given side
void get_side_verts(side_vertnum_list_t &vertlist,vcsegptr_t segnum,int sidenum);
static inline side_vertnum_list_t get_side_verts(vcsegptr_t segnum,int sidenum)
{
	side_vertnum_list_t r;
	return get_side_verts(r, segnum, sidenum), r;
}

struct vertex_array_list_t : array<int, 6> {};

#ifdef EDITOR
//      Create all vertex lists (1 or 2) for faces on a side.
//      Sets:
//              num_faces               number of lists
//              vertices                        vertices in all (1 or 2) faces
//      If there is one face, it has 4 vertices.
//      If there are two faces, they both have three vertices, so face #0 is stored in vertices 0,1,2,
//      face #1 is stored in vertices 3,4,5.
// Note: these are not absolute vertex numbers, but are relative to the segment
// Note:  for triagulated sides, the middle vertex of each trianle is the one NOT
//   adjacent on the diagonal edge
uint_fast32_t create_all_vertex_lists(vertex_array_list_t &vertices, vcsegptr_t segnum, int sidenum);
__attribute_warn_unused_result
static inline std::pair<uint_fast32_t, vertex_array_list_t> create_all_vertex_lists(vcsegptr_t segnum, int sidenum)
{
	vertex_array_list_t r;
	auto n = create_all_vertex_lists(r, segnum, sidenum);
	return {n, r};
}
#endif

//like create_all_vertex_lists(), but generate absolute point numbers
uint_fast32_t create_abs_vertex_lists(vertex_array_list_t &vertices, vcsegptr_t segnum, int sidenum);
__attribute_warn_unused_result
static inline std::pair<uint_fast32_t, vertex_array_list_t> create_abs_vertex_lists(vcsegptr_t segnum, int sidenum)
{
	vertex_array_list_t r;
	auto n = create_abs_vertex_lists(r, segnum, sidenum);
	return {n, r};
}

// -----------------------------------------------------------------------------------
// Like create all vertex lists, but returns the vertnums (relative to
// the side) for each of the faces that make up the side.
//      If there is one face, it has 4 vertices.
//      If there are two faces, they both have three vertices, so face #0 is stored in vertices 0,1,2,
//      face #1 is stored in vertices 3,4,5.
uint_fast32_t create_all_vertnum_lists(vertex_array_list_t &vertnums, vcsegptr_t segnum, int sidenum);
__attribute_warn_unused_result
static inline std::pair<uint_fast32_t, vertex_array_list_t> create_all_vertnum_lists(vcsegptr_t segnum, int sidenum)
{
	vertex_array_list_t r;
	auto n = create_all_vertnum_lists(r, segnum, sidenum);
	return {n, r};
}

//      Given a side, return the number of faces
int get_num_faces(const side *sidep);

//returns 3 different bitmasks with info telling if this sphere is in
//this segment.  See segmasks structure for info on fields
segmasks get_seg_masks(const vms_vector &checkp, vcsegptridx_t segnum, fix rad);

//this macro returns true if the segnum for an object is correct
#define check_obj_seg(obj) (get_seg_masks((obj)->pos, (obj)->segnum, 0).centermask == 0)

//Tries to find a segment for a point, in the following way:
// 1. Check the given segment
// 2. Recursively trace through attached segments
// 3. Check all the segmentns
//Returns segnum if found, or -1
segptridx_t find_point_seg(const vms_vector &p,segptridx_t segnum);

//      ----------------------------------------------------------------------------------------------------------
//      Determine whether seg0 and seg1 are reachable using wid_flag to go through walls.
//      For example, set to WID_RENDPAST_FLAG to see if sound can get from one segment to the other.
//      set to WID_FLY_FLAG to see if a robot could fly from one to the other.
//      Search up to a maximum depth of max_depth.
//      Return the distance.
struct WALL_IS_DOORWAY_mask_t;
vm_distance find_connected_distance(const vms_vector &p0, vcsegptridx_t seg0, const vms_vector &p1, vcsegptridx_t seg1, int max_depth, WALL_IS_DOORWAY_mask_t wid_flag);

//create a matrix that describes the orientation of the given segment
void extract_orient_from_segment(vms_matrix *m,vcsegptr_t seg);

//      In segment.c
//      Make a just-modified segment valid.
//              check all sides to see how many faces they each should have (0,1,2)
//              create new vector normals
void validate_segment(vsegptridx_t sp);

extern void validate_segment_all(void);

//      Extract the forward vector from segment *sp, return in *vp.
//      The forward vector is defined to be the vector from the the center of the front face of the segment
// to the center of the back face of the segment.
void extract_forward_vector_from_segment(vcsegptr_t sp,vms_vector &vp);

//      Extract the right vector from segment *sp, return in *vp.
//      The forward vector is defined to be the vector from the the center of the left face of the segment
// to the center of the right face of the segment.
void extract_right_vector_from_segment(vcsegptr_t sp,vms_vector &vp);

//      Extract the up vector from segment *sp, return in *vp.
//      The forward vector is defined to be the vector from the the center of the bottom face of the segment
// to the center of the top face of the segment.
void extract_up_vector_from_segment(vcsegptr_t sp,vms_vector &vp);

void create_walls_on_side(vsegptridx_t sp, int sidenum);

void pick_random_point_in_seg(vms_vector &new_pos, vcsegptr_t sp);
static inline vms_vector pick_random_point_in_seg(vcsegptr_t sp)
{
	vms_vector v;
	return pick_random_point_in_seg(v, sp), v;
}

void validate_segment_side(vsegptridx_t sp, int sidenum);
int check_segment_connections(void);
void flush_fcd_cache(void);
unsigned set_segment_depths(int start_seg, array<ubyte, MAX_SEGMENTS> *limit, segment_depth_array_t &depths);
void apply_all_changed_light(void);
void	set_ambient_sound_flags(void);

#endif

#endif


