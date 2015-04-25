/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 * 
 * Drawing routines
 * 
 */


#include "dxxerror.h"

#include "3d.h"
#include "globvars.h"
#include "texmap.h"
#include "clipper.h"
#ifndef OGL
#include "gr.h"
#endif

tmap_drawer_type tmap_drawer_ptr = draw_tmap;
#ifndef OGL
static const flat_drawer_type flat_drawer_ptr = gr_upoly_tmap;
static const line_drawer_type line_drawer_ptr = gr_line;
#endif

//specifies 2d drawing routines to use instead of defaults.  Passing
//NULL for either or both restores defaults
void g3_set_special_render(tmap_drawer_type tmap_drawer)
{
	tmap_drawer_ptr = tmap_drawer;
}
#ifndef OGL
//deal with a clipped line
static void must_clip_line(g3s_point *p0,g3s_point *p1,ubyte codes_or,temporary_points_t &tp)
{
	if ((p0->p3_flags&PF_TEMP_POINT) || (p1->p3_flags&PF_TEMP_POINT))
		;		//line has already been clipped, so give up
	else {
		clip_line(p0,p1,codes_or,tp);
		g3_draw_line(*p0,*p1,tp);
	}

	//free temp points

	if (p0->p3_flags & PF_TEMP_POINT)
		tp.free_temp_point(p0);

	if (p1->p3_flags & PF_TEMP_POINT)
		tp.free_temp_point(p1);
}

//draws a line. takes two points.  returns true if drew
void g3_draw_line(g3s_point &p0,g3s_point &p1)
{
	temporary_points_t tp;
	g3_draw_line(p0, p1, tp);
}

void g3_draw_line(g3s_point &p0,g3s_point &p1,temporary_points_t &tp)
{
	ubyte codes_or;

	if (p0.p3_codes & p1.p3_codes)
		return;

	codes_or = p0.p3_codes | p1.p3_codes;

	if (codes_or & CC_BEHIND)
		return must_clip_line(&p0,&p1,codes_or,tp);

	if (!(p0.p3_flags&PF_PROJECTED))
		g3_project_point(p0);

	if (p0.p3_flags&PF_OVERFLOW)
		return must_clip_line(&p0,&p1,codes_or,tp);

	if (!(p1.p3_flags&PF_PROJECTED))
		g3_project_point(p1);

	if (p1.p3_flags&PF_OVERFLOW)
		return must_clip_line(&p0,&p1,codes_or,tp);
	(*line_drawer_ptr)(p0.p3_sx,p0.p3_sy,p1.p3_sx,p1.p3_sy);
}
#endif

//returns true if a plane is facing the viewer. takes the unrotated surface 
//normal of the plane, and a point on it.  The normal need not be normalized
bool g3_check_normal_facing(const vms_vector &v,const vms_vector &norm)
{
	return (vm_vec_dot(vm_vec_sub(View_position,v),norm) > 0);
}

bool do_facing_check(const array<cg3s_point *, 3> &vertlist)
{
	//normal not specified, so must compute
		//get three points (rotated) and compute normal
		const auto tempv = vm_vec_perp(vertlist[0]->p3_vec,vertlist[1]->p3_vec,vertlist[2]->p3_vec);
		return (vm_vec_dot(tempv,vertlist[1]->p3_vec) < 0);
}

#ifndef OGL
//deal with face that must be clipped
static void must_clip_flat_face(int nv,g3s_codes cc, polygon_clip_points &Vbuf0, polygon_clip_points &Vbuf1)
{
	temporary_points_t tp;
	auto &bufptr = clip_polygon(Vbuf0,Vbuf1,&nv,&cc,tp);

	if (nv>0 && !(cc.uor&CC_BEHIND) && !cc.uand) {
		array<fix, MAX_POINTS_IN_POLY*2> Vertex_list;
		for (int i=0;i<nv;i++) {
			g3s_point *p = bufptr[i];
	
			if (!(p->p3_flags&PF_PROJECTED))
				g3_project_point(*p);
	
			if (p->p3_flags&PF_OVERFLOW) {
				goto free_points;
			}

			Vertex_list[i*2]   = p->p3_sx;
			Vertex_list[i*2+1] = p->p3_sy;
		}
		(*flat_drawer_ptr)(nv,Vertex_list);
	}
	//free temp points
free_points:
	;
}

//draw a flat-shaded face.
//returns 1 if off screen, 0 if drew
void _g3_draw_poly(uint_fast32_t nv,cg3s_point *const *const pointlist)
{
	g3s_codes cc;

	polygon_clip_points Vbuf0, Vbuf1;
	auto bufptr = &Vbuf0[0];

	for (int i=0;i<nv;i++) {

		bufptr[i] = pointlist[i];

		cc.uand &= bufptr[i]->p3_codes;
		cc.uor  |= bufptr[i]->p3_codes;
	}

	if (cc.uand)
		return;	//all points off screen

	if (cc.uor)
	{
		must_clip_flat_face(nv,cc,Vbuf0,Vbuf1);
		return;
	}

	//now make list of 2d coords (& check for overflow)
	array<fix, MAX_POINTS_IN_POLY*2> Vertex_list;
	for (int i=0;i<nv;i++) {
		g3s_point *p = bufptr[i];

		if (!(p->p3_flags&PF_PROJECTED))
			g3_project_point(*p);

		if (p->p3_flags&PF_OVERFLOW)
		{
			must_clip_flat_face(nv,cc,Vbuf0,Vbuf1);
			return;
		}

		Vertex_list[i*2]   = p->p3_sx;
		Vertex_list[i*2+1] = p->p3_sy;
	}
	(*flat_drawer_ptr)(nv,Vertex_list);
	//say it drew
}

static void must_clip_tmap_face(int nv,g3s_codes cc,grs_bitmap *bm,polygon_clip_points &Vbuf0, polygon_clip_points &Vbuf1);

//draw a texture-mapped face.
//returns 1 if off screen, 0 if drew
void _g3_draw_tmap(unsigned nv,cg3s_point *const *const pointlist,const g3s_uvl *uvl_list,const g3s_lrgb *light_rgb,grs_bitmap &bm)
{
	g3s_codes cc;

	polygon_clip_points Vbuf0, Vbuf1;
	auto bufptr = &Vbuf0[0];

	for (int i=0;i<nv;i++) {
		g3s_point *p;

		p = bufptr[i] = pointlist[i];

		cc.uand &= p->p3_codes;
		cc.uor  |= p->p3_codes;

		p->p3_u = uvl_list[i].u;
		p->p3_v = uvl_list[i].v;
		p->p3_l = (light_rgb[i].r+light_rgb[i].g+light_rgb[i].b)/3;

		p->p3_flags |= PF_UVS + PF_LS;

	}

	if (cc.uand)
		return;	//all points off screen

	if (cc.uor)
	{
		must_clip_tmap_face(nv,cc,&bm,Vbuf0,Vbuf1);
		return;
	}

	//now make list of 2d coords (& check for overflow)

	for (int i=0;i<nv;i++) {
		g3s_point *p = bufptr[i];

		if (!(p->p3_flags&PF_PROJECTED))
			g3_project_point(*p);

		if (p->p3_flags&PF_OVERFLOW) {
			Int3();		//should not overflow after clip
			return;
		}
	}

	(*tmap_drawer_ptr)(bm,nv,bufptr);
}

static void must_clip_tmap_face(int nv,g3s_codes cc,grs_bitmap *bm,polygon_clip_points &Vbuf0, polygon_clip_points &Vbuf1)
{
	temporary_points_t tp;
	auto &bufptr = clip_polygon(Vbuf0,Vbuf1,&nv,&cc,tp);
	if (nv && !(cc.uor&CC_BEHIND) && !cc.uand) {

		for (int i=0;i<nv;i++) {
			g3s_point *p = bufptr[i];

			if (!(p->p3_flags&PF_PROJECTED))
				g3_project_point(*p);
	
			if (p->p3_flags&PF_OVERFLOW) {
				Int3();		//should not overflow after clip
				goto free_points;
			}
		}

		(*tmap_drawer_ptr)(*bm,nv,&bufptr[0]);
	}

free_points:
	;

//	Assert(free_point_num==0);
}

//draw a sortof sphere - i.e., the 2d radius is proportional to the 3d
//radius, but not to the distance from the eye
void g3_draw_sphere(g3s_point &pnt,fix rad)
{
	if (! (pnt.p3_codes & CC_BEHIND)) {

		if (! (pnt.p3_flags & PF_PROJECTED))
			g3_project_point(pnt);

		if (! (pnt.p3_codes & PF_OVERFLOW)) {
			fix r2,t;

			r2 = fixmul(rad,Matrix_scale.x);
#ifndef __powerc
			if (checkmuldiv(&t,r2,Canv_w2,pnt.p3_z))
			{
				gr_disk(pnt.p3_sx,pnt.p3_sy,t);
				return;
			}
#else
			if (pnt.p3_z == 0)
				return;
			gr_disk(pnt.p3_sx, pnt.p3_sy, fl2f(((f2fl(r2) * fCanv_w2) / f2fl(pnt.p3_z))));
			return;
#endif
		}
	}
}
#endif

