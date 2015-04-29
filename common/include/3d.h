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
 * Header file for 3d library
 * except for functions implemented in interp.c
 *
 */

#pragma once

#include <cstdint>
#include "dxxsconf.h"
#include "maths.h"
#include "vecmat.h" //the vector/matrix library

#include "compiler-array.h"

struct grs_bitmap;

#ifdef EDITOR
extern int g3d_interp_outline;      //if on, polygon models outlined in white
#endif

//Structure for storing u,v,light values.  This structure doesn't have a
//prefix because it was defined somewhere else before it was moved here
struct g3s_uvl {
	fix u,v,l;
};

//Structure for storing light color. Also uses l of g3s-uvl to add/compute mono (white) light
struct g3s_lrgb {
	fix r,g,b;
};

//Stucture to store clipping codes in a word
struct g3s_codes {
	ubyte uor,uand;   //or is low byte, and is high byte
	constexpr g3s_codes() :
		uor(0), uand(0xff)
	{
	}
};

//flags for point structure
const uint8_t PF_PROJECTED = 1;		//has been projected, so sx,sy valid
const uint8_t PF_OVERFLOW = 2;		//can't project
const uint8_t PF_TEMP_POINT = 4;	//created during clip
const uint8_t PF_UVS = 8;			//has uv values set
const uint8_t PF_LS = 16;			//has lighting values set

//clipping codes flags

const uint8_t CC_OFF_LEFT = 1;
const uint8_t CC_OFF_RIGHT = 2;
const uint8_t CC_OFF_BOT = 4;
const uint8_t CC_OFF_TOP = 8;
const uint8_t CC_BEHIND = 0x80;

//Used to store rotated points for mines.  Has frame count to indictate
//if rotated, and flag to indicate if projected.
struct g3s_point {
	vms_vector p3_vec;  //x,y,z of rotated point
	fix p3_u,p3_v,p3_l; //u,v,l coords
	fix p3_sx,p3_sy;    //screen x&y
	ubyte p3_codes;     //clipping codes
	ubyte p3_flags;     //projected?
	uint16_t p3_last_generation;
};

//macros to reference x,y,z elements of a 3d point
#define p3_x p3_vec.x
#define p3_y p3_vec.y
#define p3_z p3_vec.z

//An object, such as a robot
struct g3s_object {
	vms_vector o3_pos;       //location of this object
	vms_angvec o3_orient;    //orientation of this object
	int o3_nverts;           //number of points in the object
	int o3_nfaces;           //number of faces in the object

	//this will be filled in later
};

#ifdef __cplusplus
//Functions in library

//Frame setup functions:

#ifdef OGL
typedef const g3s_point cg3s_point;
#else
typedef g3s_point cg3s_point;
#endif

//start the frame
void g3_start_frame(void);

//set view from x,y,z, viewer matrix, and zoom.  Must call one of g3_set_view_*() 
void g3_set_view_matrix(const vms_vector &view_pos,const vms_matrix &view_matrix,fix zoom);

//end the frame
#ifdef OGL
#define g3_end_frame() ogl_end_frame()
#else
#define g3_end_frame()
#endif

//draw a horizon
void g3_draw_horizon(int sky_color,int ground_color);

//Instancing

//instance at specified point with specified orientation
void g3_start_instance_matrix(const vms_vector &pos,const vms_matrix *orient);

//instance at specified point with specified orientation
void g3_start_instance_angles(const vms_vector &pos,const vms_angvec *angles);

//pops the old context
void g3_done_instance();

//Misc utility functions:

//get zoom.  For a given window size, return the zoom which will achieve
//the given FOV along the given axis.
fix g3_get_zoom(char axis,fixang fov,short window_width,short window_height);

//returns true if a plane is facing the viewer. takes the unrotated surface 
//normal of the plane, and a point on it.  The normal need not be normalized
bool g3_check_normal_facing(const vms_vector &v,const vms_vector &norm);

//Point definition and rotation functions:

//specify the arrays refered to by the 'pointlist' parms in the following
//functions.  I'm not sure if we will keep this function, but I need
//it now.
//void g3_set_points(g3s_point *points,vms_vector *vecs);

//returns codes_and & codes_or of a list of points numbers
g3s_codes g3_check_codes(int nv,g3s_point **pointlist);

//rotates a point. returns codes.  does not check if already rotated
ubyte g3_rotate_point(g3s_point &dest,const vms_vector &src);
static inline g3s_point g3_rotate_point(const vms_vector &src) __attribute_warn_unused_result;
static inline g3s_point g3_rotate_point(const vms_vector &src)
{
	g3s_point dest;
	return g3_rotate_point(dest, src), dest;
}

//projects a point
void g3_project_point(g3s_point &point);

//calculate the depth of a point - returns the z coord of the rotated point
fix g3_calc_point_depth(const vms_vector &pnt);

//from a 2d point, compute the vector through that point
void g3_point_2_vec(vms_vector &v,short sx,short sy);

//code a point.  fills in the p3_codes field of the point, and returns the codes
ubyte g3_code_point(g3s_point &point);

//delta rotation functions
void g3_rotate_delta_vec(vms_vector &dest,const vms_vector &src);
ubyte g3_add_delta_vec(g3s_point &dest,const g3s_point &src,const vms_vector &deltav);

//Drawing functions:

//draw a flat-shaded face.
//returns 1 if off screen, 0 if drew
void _g3_draw_poly(uint_fast32_t nv,cg3s_point *const *pointlist);
template <std::size_t N>
static inline void g3_draw_poly(uint_fast32_t nv, const array<cg3s_point *, N> &pointlist)
{
	_g3_draw_poly(nv, &pointlist[0]);
}

template <std::size_t N>
static inline void g3_draw_poly(const array<cg3s_point *, N> &pointlist)
{
	g3_draw_poly(N, pointlist);
}

static const std::size_t MAX_POINTS_PER_POLY = 25;

//draw a texture-mapped face.
//returns 1 if off screen, 0 if drew
void _g3_draw_tmap(unsigned nv, cg3s_point *const *pointlist, const g3s_uvl *uvl_list, const g3s_lrgb *light_rgb, grs_bitmap &bm);

template <std::size_t N>
static inline void g3_draw_tmap(unsigned nv, const array<cg3s_point *, N> &pointlist, const array<g3s_uvl, N> &uvl_list, const array<g3s_lrgb, N> &light_rgb, grs_bitmap &bm)
{
	static_assert(N <= MAX_POINTS_PER_POLY, "too many points in tmap");
#ifdef DXX_HAVE_BUILTIN_CONSTANT_P
	if (__builtin_constant_p(nv > N) && nv > N)
		DXX_ALWAYS_ERROR_FUNCTION(dxx_trap_tmap_overread, "reading beyond array");
#endif
	_g3_draw_tmap(nv, &pointlist[0], &uvl_list[0], &light_rgb[0], bm);
}

template <std::size_t N>
static inline void g3_draw_tmap(const array<cg3s_point *, N> &pointlist, const array<g3s_uvl, N> &uvl_list, const array<g3s_lrgb, N> &light_rgb, grs_bitmap &bm)
{
	g3_draw_tmap(N, pointlist, uvl_list, light_rgb, bm);
}

//draw a sortof sphere - i.e., the 2d radius is proportional to the 3d
//radius, but not to the distance from the eye
void g3_draw_sphere(g3s_point &pnt,fix rad);

//@@//return ligting value for a point
//@@fix g3_compute_lighting_value(g3s_point *rotated_point,fix normval);


//like g3_draw_poly(), but checks to see if facing.  If surface normal is
//NULL, this routine must compute it, which will be slow.  It is better to 
//pre-compute the normal, and pass it to this function.  When the normal
//is passed, this function works like g3_check_normal_facing() plus
//g3_draw_poly().
//returns -1 if not facing, 1 if off screen, 0 if drew
bool do_facing_check(const array<cg3s_point *, 3> &vertlist);

//like g3_draw_poly(), but checks to see if facing.  If surface normal is
//NULL, this routine must compute it, which will be slow.  It is better to 
//pre-compute the normal, and pass it to this function.  When the normal
//is passed, this function works like g3_check_normal_facing() plus
//g3_draw_poly().
//returns -1 if not facing, 1 if off screen, 0 if drew
static inline void g3_check_and_draw_poly(const array<cg3s_point *, 3> &pointlist)
{
	if (do_facing_check(pointlist))
		g3_draw_poly(pointlist);
}

template <std::size_t N>
static inline void g3_check_and_draw_tmap(unsigned nv, const array<cg3s_point *, N> &pointlist, const array<g3s_uvl, N> &uvl_list, const array<g3s_lrgb, N> &light_rgb, grs_bitmap &bm)
{
	if (do_facing_check(pointlist))
		g3_draw_tmap(nv,pointlist,uvl_list,light_rgb,bm);
}

template <std::size_t N>
static inline void g3_check_and_draw_tmap(const array<cg3s_point *, N> &pointlist, const array<g3s_uvl, N> &uvl_list, const array<g3s_lrgb, N> &light_rgb, grs_bitmap &bm)
{
	g3_check_and_draw_tmap(N, pointlist, uvl_list, light_rgb, bm);
}

//draws a line. takes two points.
struct temporary_points_t;
void g3_draw_line(cg3s_point &p0,cg3s_point &p1);
void g3_draw_line(cg3s_point &p0,cg3s_point &p1,temporary_points_t &);

//draw a bitmap object that is always facing you
//returns 1 if off screen, 0 if drew
void g3_draw_rod_tmap(grs_bitmap &bitmap,const g3s_point &bot_point,fix bot_width,const g3s_point &top_point,fix top_width,g3s_lrgb light);

//draws a bitmap with the specified 3d width & height
//returns 1 if off screen, 0 if drew
void g3_draw_bitmap(const vms_vector &pos,fix width,fix height,grs_bitmap &bm);

//specifies 2d drawing routines to use instead of defaults.  Passing
//NULL for either or both restores defaults
#ifdef OGL
template <uint_fast8_t type>
class tmap_drawer_constant
{
};

const tmap_drawer_constant<0> draw_tmap{};
const tmap_drawer_constant<1> draw_tmap_flat{};

class tmap_drawer_type
{
	uint_fast8_t type;
public:
	template <uint_fast8_t t>
		constexpr tmap_drawer_type(tmap_drawer_constant<t>) : type(t)
	{
	}
	template <uint_fast8_t t>
		bool operator==(tmap_drawer_constant<t>) const
		{
			return type == t;
		}
	template <uint_fast8_t t>
		bool operator!=(tmap_drawer_constant<t>) const
		{
			return type != t;
		}
};
#else
constexpr std::size_t MAX_POINTS_IN_POLY = 100;

typedef void (*tmap_drawer_type)(const grs_bitmap &bm,uint_fast32_t nv,const g3s_point *const *vertlist);
typedef void (*flat_drawer_type)(uint_fast32_t nv,const array<fix, MAX_POINTS_IN_POLY*2> &vertlist);
typedef void (*line_drawer_type)(fix x0,fix y0,fix x1,fix y1);

//	This is the gr_upoly-like interface to the texture mapper which uses texture-mapper compatible
//	(ie, avoids cracking) edge/delta computation.
void gr_upoly_tmap(uint_fast32_t nverts, const array<fix, MAX_POINTS_IN_POLY*2> &vert);
#endif
void g3_set_special_render(tmap_drawer_type tmap_drawer);

extern tmap_drawer_type tmap_drawer_ptr;

#endif
