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
 * This will contain the laser code
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "inferno.h"
#include "game.h"
#include "bm.h"
#include "object.h"
#include "laser.h"
#include "args.h"
#include "segment.h"
#include "fvi.h"
#include "segpoint.h"
#include "dxxerror.h"
#include "key.h"
#include "texmap.h"
#include "gameseg.h"
#include "textures.h"
#include "render.h"
#include "vclip.h"
#include "fireball.h"
#include "polyobj.h"
#include "robot.h"
#include "weapon.h"
#include "newdemo.h"
#include "timer.h"
#include "player.h"
#include "sounds.h"
#include "ai.h"
#include "powerup.h"
#include "multi.h"
#include "physics.h"
#include "multi.h"
#include "fwdwall.h"
#include "reverse.h"

#include "compiler-range_for.h"
#include "highest_valid.h"
#include "partial_range.h"

#define NEWHOMER

#if defined(DXX_BUILD_DESCENT_II)
array<object *, MAX_PLAYERS> Guided_missile;
array<object_signature_t, MAX_PLAYERS> Guided_missile_sig;
#endif
objnum_t Network_laser_track = object_none;

static objptridx_t find_homing_object_complete(const vms_vector &curpos, const vobjptridx_t tracker, int track_obj_type1, int track_obj_type2);
static objptridx_t find_homing_object(const vms_vector &curpos, const vobjptridx_t tracker);

//---------------------------------------------------------------------------------
// Called by render code.... determines if the laser is from a robot or the
// player and calls the appropriate routine.

void Laser_render(const vobjptr_t obj)
{

//	Commented out by John (sort of, typed by Mike) on 6/8/94
#if 0
	switch( obj->id )	{
	case WEAPON_TYPE_WEAK_LASER:
	case WEAPON_TYPE_STRONG_LASER:
	case WEAPON_TYPE_CANNON_BALL:
	case WEAPON_TYPE_MISSILE:
		break;
	default:
		Error( "Invalid weapon type in Laser_render\n" );
	}
#endif
	auto &wi = Weapon_info[get_weapon_id(obj)];
	switch(wi.render_type)
	{
	case WEAPON_RENDER_LASER:
		Int3();	// Not supported anymore!
					//Laser_draw_one(obj-Objects, Weapon_info[obj->id].bitmap );
		break;
	case WEAPON_RENDER_BLOB:
		draw_object_blob(obj, wi.bitmap);
		break;
	case WEAPON_RENDER_POLYMODEL:
		break;
	case WEAPON_RENDER_VCLIP:
		Int3();	//	Oops, not supported, type added by mk on 09/09/94, but not for lasers...
	default:
		Error( "Invalid weapon render type in Laser_render\n" );
	}

}

//---------------------------------------------------------------------------------
// Draws a texture-mapped laser bolt

//void Laser_draw_one( int objnum, grs_bitmap * bmp )
//{
//	int t1, t2, t3;
//	g3s_point p1, p2;
//	object *obj;
//	vms_vector start_pos,end_pos;
//
//	obj = &Objects[objnum];
//
//	start_pos = obj->pos;
//	vm_vec_scale_add(&end_pos,&start_pos,&obj->orient.fvec,-Laser_length);
//
//	g3_rotate_point(&p1,&start_pos);
//	g3_rotate_point(&p2,&end_pos);
//
//	t1 = Lighting_on;
//	t2 = Interpolation_method;
//	t3 = Transparency_on;
//
//	Lighting_on  = 0;
//	//Interpolation_method = 3;	// Full perspective
//	Interpolation_method = 1;	// Linear
//	Transparency_on = 1;
//
//	//gr_setcolor( gr_getcolor(31,15,0));
//	//g3_draw_line_ptrs(p1,p2);
//	//g3_draw_rod(p1,0x2000,p2,0x2000);
//	//g3_draw_rod(p1,Laser_width,p2,Laser_width);
//	g3_draw_rod_tmap(bmp,&p2,Laser_width,&p1,Laser_width,0);
//	Lighting_on = t1;
//	Interpolation_method = t2;
//	Transparency_on = t3;
//
//}

//	Changed by MK on 09/07/94
//	I want you to be able to blow up your own bombs.
//	AND...Your proximity bombs can blow you up if they're 2.0 seconds or more old.
//	Changed by MK on 06/06/95: Now must be 4.0 seconds old.  Much valid Net-complaining.
int laser_are_related( int o1, int o2 )
{
	if ( (o1<0) || (o2<0) )
		return 0;

	// See if o2 is the parent of o1
	if ( Objects[o1].type == OBJ_WEAPON  )
		if ( (Objects[o1].ctype.laser_info.parent_num==o2) && (Objects[o1].ctype.laser_info.parent_signature==Objects[o2].signature) )
		{
			//	o1 is a weapon, o2 is the parent of 1, so if o1 is PROXIMITY_BOMB and o2 is player, they are related only if o1 < 2.0 seconds old
#if defined(DXX_BUILD_DESCENT_I)
			if (!((get_weapon_id(&Objects[o1]) != PROXIMITY_ID) || (Objects[o1].ctype.laser_info.creation_time + F1_0*2 >= GameTime64)))
#elif defined(DXX_BUILD_DESCENT_II)
			if ((get_weapon_id(&Objects[o1]) == PHOENIX_ID && (GameTime64 > Objects[o1].ctype.laser_info.creation_time + F1_0/4)) ||
			   (get_weapon_id(&Objects[o1]) == GUIDEDMISS_ID && (GameTime64 > Objects[o1].ctype.laser_info.creation_time + F1_0*2)) ||
				(((get_weapon_id(&Objects[o1]) == PROXIMITY_ID) || (get_weapon_id(&Objects[o1]) == SUPERPROX_ID)) && (GameTime64 > Objects[o1].ctype.laser_info.creation_time + F1_0*4)))
#endif
			{
				return 0;
			} else
				return 1;
		}

	// See if o1 is the parent of o2
	if ( Objects[o2].type == OBJ_WEAPON  )
	{
		if ( (Objects[o2].ctype.laser_info.parent_num==o1) && (Objects[o2].ctype.laser_info.parent_signature==Objects[o1].signature) )
		{
#if defined(DXX_BUILD_DESCENT_II)
			//	o2 is a weapon, o1 is the parent of 2, so if o2 is PROXIMITY_BOMB and o1 is player, they are related only if o1 < 2.0 seconds old
			if ((Objects[o2].id == PHOENIX_ID && (GameTime64 > Objects[o2].ctype.laser_info.creation_time + F1_0/4)) ||
			   (Objects[o2].id == GUIDEDMISS_ID && (GameTime64 > Objects[o2].ctype.laser_info.creation_time + F1_0*2)) ||
				(((Objects[o2].id == PROXIMITY_ID) || (Objects[o2].id == SUPERPROX_ID)) && (GameTime64 > Objects[o2].ctype.laser_info.creation_time + F1_0*4))) {
				return 0;
			} else
#endif
				return 1;
		}
	}

	// They must both be weapons
	if ( Objects[o1].type != OBJ_WEAPON || Objects[o2].type != OBJ_WEAPON )
		return 0;

	//	Here is the 09/07/94 change -- Siblings must be identical, others can hurt each other
	// See if they're siblings...
	//	MK: 06/08/95, Don't allow prox bombs to detonate for 3/4 second.  Else too likely to get toasted by your own bomb if hit by opponent.
	if ( Objects[o1].ctype.laser_info.parent_signature==Objects[o2].ctype.laser_info.parent_signature )
	{
		if (is_proximity_bomb_or_smart_mine(get_weapon_id(&Objects[o1])) || is_proximity_bomb_or_smart_mine(get_weapon_id(&Objects[o2]))) {
			//	If neither is older than 1/2 second, then can't blow up!
#if defined(DXX_BUILD_DESCENT_II)
			if (!((GameTime64 > (Objects[o1].ctype.laser_info.creation_time + F1_0/2)) || (GameTime64 > (Objects[o2].ctype.laser_info.creation_time + F1_0/2))))
				return 1;
			else
#endif
				return 0;
		} else
			return 1;
	}

#if defined(DXX_BUILD_DESCENT_II)
	//	Anything can cause a collision with a robot super prox mine.
	if (!(Objects[o1].id == ROBOT_SUPERPROX_ID || Objects[o2].id == ROBOT_SUPERPROX_ID ||
		 Objects[o1].id == PROXIMITY_ID || Objects[o2].id == PROXIMITY_ID ||
		 Objects[o1].id == SUPERPROX_ID || Objects[o2].id == SUPERPROX_ID ||
		 Objects[o1].id == PMINE_ID || Objects[o2].id == PMINE_ID))
		return 1;
#endif
	return 0;
}

static void do_muzzle_stuff(segnum_t segnum, const vms_vector &pos)
{
	Muzzle_data[Muzzle_queue_index].create_time = timer_query();
	Muzzle_data[Muzzle_queue_index].segnum = segnum;
	Muzzle_data[Muzzle_queue_index].pos = pos;
	Muzzle_queue_index++;
	if (Muzzle_queue_index >= MUZZLE_QUEUE_MAX)
		Muzzle_queue_index = 0;
}

//creates a weapon object
static objptridx_t create_weapon_object(int weapon_type,const vsegptridx_t segnum, const vms_vector &position)
{
	int rtype=-1;
	fix laser_radius = -1;

	switch( Weapon_info[weapon_type].render_type )	{

		case WEAPON_RENDER_BLOB:
			rtype = RT_LASER;			// Render as a laser even if blob (see render code above for explanation)
			laser_radius = Weapon_info[weapon_type].blob_size;
			break;
		case WEAPON_RENDER_POLYMODEL:
			laser_radius = 0;	//	Filled in below.
			rtype = RT_POLYOBJ;
			break;
		case WEAPON_RENDER_LASER:
			Int3(); 	// Not supported anymore
			break;
		case WEAPON_RENDER_NONE:
			rtype = RT_NONE;
			laser_radius = F1_0;
			break;
		case WEAPON_RENDER_VCLIP:
			rtype = RT_WEAPON_VCLIP;
			laser_radius = Weapon_info[weapon_type].blob_size;
			break;
		default:
			Error( "Invalid weapon render type in Laser_create_new\n" );
	}

	Assert(laser_radius != -1);
	Assert(rtype != -1);

	auto obj = obj_create( OBJ_WEAPON, weapon_type, segnum, position, NULL, laser_radius, CT_WEAPON, MT_PHYSICS, rtype );
	if (obj == object_none)
		return obj;

	if (Weapon_info[weapon_type].render_type == WEAPON_RENDER_POLYMODEL) {
		obj->rtype.pobj_info.model_num = Weapon_info[get_weapon_id(obj)].model_num;
		obj->size = fixdiv(Polygon_models[obj->rtype.pobj_info.model_num].rad,Weapon_info[get_weapon_id(obj)].po_len_to_width_ratio);
	}

	obj->mtype.phys_info.mass = Weapon_info[weapon_type].mass;
	obj->mtype.phys_info.drag = Weapon_info[weapon_type].drag;
	vm_vec_zero(obj->mtype.phys_info.thrust);

	if (Weapon_info[weapon_type].bounce==1)
		obj->mtype.phys_info.flags |= PF_BOUNCE;

#if defined(DXX_BUILD_DESCENT_II)
	if (Weapon_info[weapon_type].bounce==2 || cheats.bouncyfire)
		obj->mtype.phys_info.flags |= PF_BOUNCE+PF_BOUNCES_TWICE;
#endif


	return obj;
}

#if defined(DXX_BUILD_DESCENT_II)
//	-------------------------------------------------------------------------------------------------------------------------------
//	***** HEY ARTISTS!! *****
//	Here are the constants you're looking for! --MK

//	Change the following constants to affect the look of the omega cannon.
//	Changing these constants will not affect the damage done.
//	WARNING: If you change DESIRED_OMEGA_DIST and MAX_OMEGA_BLOBS, you don't merely change the look of the cannon,
//	you change its range.  If you decrease DESIRED_OMEGA_DIST, you decrease how far the gun can fire.
const fix OMEGA_BASE_TIME = F1_0/20; // How many blobs per second!! No FPS-based blob creation anymore, no FPS-based damage anymore!
const unsigned MIN_OMEGA_BLOBS = 3;				//	No matter how close the obstruction, at this many blobs created.
const fix MIN_OMEGA_DIST = F1_0*3;		//	At least this distance between blobs, unless doing so would violate MIN_OMEGA_BLOBS
const fix DESIRED_OMEGA_DIST = F1_0*5;		//	This is the desired distance between blobs.  For distances > MIN_OMEGA_BLOBS*DESIRED_OMEGA_DIST, but not very large, this will apply.
const unsigned MAX_OMEGA_BLOBS = 16;				//	No matter how far away the obstruction, this is the maximum number of blobs.
constexpr vm_distance MAX_OMEGA_DIST{MAX_OMEGA_BLOBS * DESIRED_OMEGA_DIST};		//	Maximum extent of lightning blobs.
constexpr vm_distance_squared MAX_OMEGA_DIST_SQUARED{MAX_OMEGA_DIST * MAX_OMEGA_DIST};

//	Additionally, several constants which apply to homing objects in general control the behavior of the Omega Cannon.
//	They are defined in laser.h.  They are copied here for reference.  These values are valid on 1/10/96:
//	If you want the Omega Cannon view cone to be different than the Homing Missile viewcone, contact MK to make the change.
//	(Unless you are a programmer, in which case, do it yourself!)
#define	OMEGA_MIN_TRACKABLE_DOT			(15*F1_0/16)		//	Larger values mean narrower cone.  F1_0 means damn near impossible.  0 means 180 degree field of view.
constexpr vm_distance OMEGA_MAX_TRACKABLE_DIST = MAX_OMEGA_DIST; //	An object must be at least this close to be tracked.

//	Note, you don't need to change these constants.  You can control damage and energy consumption by changing the
//	usual bitmaps.tbl parameters.
#define	OMEGA_DAMAGE_SCALE			32				//	Controls how much damage is done.  This gets multiplied by the damage specified in bitmaps.tbl in the $WEAPON line.
#define	OMEGA_ENERGY_CONSUMPTION	16				//	Controls how much energy is consumed.  This gets multiplied by the energy parameter from bitmaps.tbl.
//	-------------------------------------------------------------------------------------------------------------------------------

// Delete omega blobs further away than MAX_OMEGA_DIST
// Since last omega blob has VERY high velocity it's impossible to ensure a constant travel distance on varying FPS. So delete if they exceed their maximum distance.
static int omega_cleanup(const vobjptridx_t weapon)
{
	if (weapon->type != OBJ_WEAPON || weapon->id != OMEGA_ID)
		return 0;
	const auto parent_sig = weapon->ctype.laser_info.parent_signature;
	const auto parent_num = weapon->ctype.laser_info.parent_num;
	if (Objects[parent_num].signature == parent_sig)
		if (vm_vec_dist2(weapon->pos, Objects[parent_num].pos) > MAX_OMEGA_DIST_SQUARED)
		{
			obj_delete(weapon);
			return 1;
		}

	return 0;
}

// Return true if ok to do Omega damage. For Multiplayer games. See comment for omega_cleanup()
int ok_to_do_omega_damage(const vcobjptr_t weapon)
{
	if (weapon->type != OBJ_WEAPON || weapon->id != OMEGA_ID)
		return 1;
	if (!(Game_mode & GM_MULTI))
		return 1;
	const auto parent_sig = weapon->ctype.laser_info.parent_signature;
	const auto parent_num = weapon->ctype.laser_info.parent_num;
	if (Objects[parent_num].signature == parent_sig)
		if (vm_vec_dist2(Objects[parent_num].pos, weapon->pos) > MAX_OMEGA_DIST_SQUARED)
			return 0;

	return 1;
}

// ---------------------------------------------------------------------------------
static void create_omega_blobs(const segptridx_t firing_segnum, const vms_vector &firing_pos, const vms_vector &goal_pos, const vobjptridx_t parent_objp)
{
	objptridx_t  last_created_objnum = object_none;
	vms_vector	omega_delta_vector = ZERO_VECTOR, blob_pos = ZERO_VECTOR;
	fix		dist_to_goal = 0, omega_blob_dist = 0, perturb_array[MAX_OMEGA_BLOBS]{};

	auto vec_to_goal = vm_vec_sub(goal_pos, firing_pos);
	dist_to_goal = vm_vec_normalize_quick(vec_to_goal);

	unsigned num_omega_blobs = 0;
	if (dist_to_goal < MIN_OMEGA_BLOBS * MIN_OMEGA_DIST) {
		omega_blob_dist = MIN_OMEGA_DIST;
		num_omega_blobs = dist_to_goal/omega_blob_dist;
		if (num_omega_blobs == 0)
			num_omega_blobs = 1;
	} else {
		omega_blob_dist = DESIRED_OMEGA_DIST;
		num_omega_blobs = dist_to_goal / omega_blob_dist;
		if (num_omega_blobs > MAX_OMEGA_BLOBS) {
			num_omega_blobs = MAX_OMEGA_BLOBS;
			omega_blob_dist = dist_to_goal / num_omega_blobs;
		} else if (num_omega_blobs < MIN_OMEGA_BLOBS) {
			num_omega_blobs = MIN_OMEGA_BLOBS;
			omega_blob_dist = dist_to_goal / num_omega_blobs;
		}
	}

	omega_delta_vector = vec_to_goal;
	vm_vec_scale(omega_delta_vector, omega_blob_dist);

	//	Now, create all the blobs
	blob_pos = firing_pos;
	auto last_segnum = firing_segnum;

	//	If nearby, don't perturb vector.  If not nearby, start halfway out.
	if (dist_to_goal < MIN_OMEGA_DIST*4) {
		range_for (auto &i, partial_range(perturb_array, num_omega_blobs))
			i = 0;
	} else {
		vm_vec_scale_add2(blob_pos, omega_delta_vector, F1_0/2);	//	Put first blob half way out.
		for (int i=0; i<num_omega_blobs/2; i++) {
			perturb_array[i] = F1_0*i + F1_0/4;
			perturb_array[num_omega_blobs-1-i] = F1_0*i;
		}
	}

	//	Create random perturbation vector, but favor _not_ going up in player's reference.
	auto perturb_vec = make_random_vector();
	vm_vec_scale_add2(perturb_vec, parent_objp->orient.uvec, -F1_0/2);

	Doing_lighting_hack_flag = 1;	//	Ugly, but prevents blobs which are probably outside the mine from killing framerate.

	for (int i=0; i<num_omega_blobs; i++) {
		//	This will put the last blob right at the destination object, causing damage.
		if (i == num_omega_blobs-1)
			vm_vec_scale_add2(blob_pos, omega_delta_vector, 15*F1_0/32);	//	Move last blob another (almost) half section

		//	Every so often, re-perturb blobs
		if ((i % 4) == 3) {
			const auto temp_vec = make_random_vector();
			vm_vec_scale_add2(perturb_vec, temp_vec, F1_0/4);
		}

		const auto temp_pos = vm_vec_scale_add(blob_pos, perturb_vec, perturb_array[i]);

		auto segnum = find_point_seg(temp_pos, last_segnum);
		if (segnum != segment_none) {
			last_segnum = segnum;
			auto blob_objnum = obj_create(OBJ_WEAPON, OMEGA_ID, segnum, temp_pos, NULL, 0, CT_WEAPON, MT_PHYSICS, RT_WEAPON_VCLIP );
			if (blob_objnum == object_none)
				break;

			last_created_objnum = blob_objnum;

			auto &objp = blob_objnum;

			objp->lifeleft = OMEGA_BASE_TIME+(d_rand()/8); // add little randomness so the lighting effect becomes a little more interesting
			objp->mtype.phys_info.velocity = vec_to_goal;

			//	Only make the last one move fast, else multiple blobs might collide with target.
			vm_vec_scale(objp->mtype.phys_info.velocity, F1_0*4);

			objp->size = Weapon_info[objp->id].blob_size;

			objp->shields = fixmul(OMEGA_DAMAGE_SCALE*OMEGA_BASE_TIME, Weapon_info[objp->id].strength[Difficulty_level]);

			objp->ctype.laser_info.parent_type			= parent_objp->type;
			objp->ctype.laser_info.parent_signature	= parent_objp->signature;
			objp->ctype.laser_info.parent_num			= parent_objp;
			objp->movement_type = MT_NONE;	//	Only last one moves, that will get bashed below.

		}
		vm_vec_add2(blob_pos, omega_delta_vector);
	}

	//	Make last one move faster, but it's already moving at speed = F1_0*4.
	if (last_created_objnum != object_none) {
		vm_vec_scale(last_created_objnum->mtype.phys_info.velocity, Weapon_info[OMEGA_ID].speed[Difficulty_level]/4);
		last_created_objnum->movement_type = MT_PHYSICS;
	}

	Doing_lighting_hack_flag = 0;
}

#define	MIN_OMEGA_CHARGE	(MAX_OMEGA_CHARGE/8)
#define	OMEGA_CHARGE_SCALE	4			//	FrameTime / OMEGA_CHARGE_SCALE added to Omega_charge every frame.
fix	Omega_charge = MAX_OMEGA_CHARGE;

#define	OMEGA_CHARGE_SCALE	4

int	Last_omega_fire_time=0;

// ---------------------------------------------------------------------------------
//	Call this every frame to recharge the Omega Cannon.
void omega_charge_frame(void)
{
	fix	delta_charge, old_omega_charge;

	if (Omega_charge == MAX_OMEGA_CHARGE)
		return;

	if (!player_has_primary_weapon(primary_weapon_index_t::OMEGA_INDEX).has_weapon())
		return;

	if (Player_is_dead)
		return;

	//	Don't charge while firing. Wait 1/3 second after firing before recharging
	if (Last_omega_fire_time > GameTime64)
		Last_omega_fire_time = GameTime64;
	if (Last_omega_fire_time + F1_0/3 > GameTime64)
		return;

	if (Players[Player_num].energy) {
		fix	energy_used;

		old_omega_charge = Omega_charge;
		Omega_charge += FrameTime/OMEGA_CHARGE_SCALE;
		if (Omega_charge > MAX_OMEGA_CHARGE)
			Omega_charge = MAX_OMEGA_CHARGE;

		delta_charge = Omega_charge - old_omega_charge;

		energy_used = fixmul(F1_0*190/17, delta_charge);
		if (Difficulty_level < 2)
			energy_used = fixmul(energy_used, i2f(Difficulty_level+2)/4);

		Players[Player_num].energy -= energy_used;
		if (Players[Player_num].energy < 0)
			Players[Player_num].energy = 0;
	}


}

// -- fix	Last_omega_muzzle_flash_time;

// ---------------------------------------------------------------------------------
//	*objp is the object firing the omega cannon
//	*pos is the location from which the omega bolt starts
static void do_omega_stuff(const vobjptridx_t parent_objp, const vms_vector &firing_pos, const vobjptridx_t weapon_objp)
{
	objnum_t			lock_objnum;
	vms_vector	goal_pos;
	int			pnum = parent_objp->id;
	fix fire_frame_overhead = 0;

	if (pnum == Player_num) {
		//	If charge >= min, or (some charge and zero energy), allow to fire.
		if (!((Omega_charge >= MIN_OMEGA_CHARGE) || (Omega_charge && !Players[pnum].energy))) {
			obj_delete(weapon_objp);
			return;
		}

		Omega_charge -= OMEGA_BASE_TIME;
		if (Omega_charge < 0)
			Omega_charge = 0;

		if (GameTime64 - Last_omega_fire_time + OMEGA_BASE_TIME <= FrameTime) // if firing is prolonged by FrameTime overhead, let's try to fix that. Since Next_laser_firing_time is probably changed already (in do_laser_firing_player), we need to calculate the overhead slightly different. 
			fire_frame_overhead = GameTime64 - Last_omega_fire_time + OMEGA_BASE_TIME;

		Next_laser_fire_time = GameTime64+OMEGA_BASE_TIME-fire_frame_overhead;
		Last_omega_fire_time = GameTime64;
	}

	weapon_objp->ctype.laser_info.parent_type = OBJ_PLAYER;
	weapon_objp->ctype.laser_info.parent_num = Players[pnum].objnum;
	weapon_objp->ctype.laser_info.parent_signature = Objects[Players[pnum].objnum].signature;

	lock_objnum = find_homing_object(firing_pos, weapon_objp);

	auto firing_segnum = find_point_seg(firing_pos, parent_objp->segnum);

	//	Play sound.
	if ( parent_objp == Viewer )
		digi_play_sample( Weapon_info[weapon_objp->id].flash_sound, F1_0 );
	else
		digi_link_sound_to_pos( Weapon_info[weapon_objp->id].flash_sound, weapon_objp->segnum, 0, weapon_objp->pos, 0, F1_0 );

	// -- if ((Last_omega_muzzle_flash_time + F1_0/4 < GameTime) || (Last_omega_muzzle_flash_time > GameTime)) {
	// -- 	do_muzzle_stuff(firing_segnum, firing_pos);
	// -- 	Last_omega_muzzle_flash_time = GameTime;
	// -- }

	//	Delete the original object.  Its only purpose in life was to determine which object to home in on.
	obj_delete(weapon_objp);

	//	If couldn't lock on anything, fire straight ahead.
	if (lock_objnum == object_none) {
		fvi_query	fq;
		fvi_info		hit_data;
		int			fate;
		const auto perturb_vec = make_random_vector();
		const auto perturbed_fvec = vm_vec_scale_add(parent_objp->orient.fvec, perturb_vec, F1_0/16);
		vm_vec_scale_add(goal_pos, firing_pos, perturbed_fvec, MAX_OMEGA_DIST);
		fq.startseg = firing_segnum;
		if (fq.startseg == segment_none) {
			return;
		}
		fq.p0						= &firing_pos;
		fq.p1						= &goal_pos;
		fq.rad					= 0;
		fq.thisobjnum			= parent_objp;
		fq.ignore_obj_list.first = nullptr;
		fq.flags					= FQ_IGNORE_POWERUPS | FQ_TRANSPOINT | FQ_CHECK_OBJS;		//what about trans walls???

		fate = find_vector_intersection(fq, hit_data);
		if (fate != HIT_NONE) {
			Assert(hit_data.hit_seg != segment_none);		//	How can this be?  We went from inside the mine to outside without hitting anything?
			goal_pos = hit_data.hit_pnt;
		}
	} else
		goal_pos = Objects[lock_objnum].pos;

	//	This is where we create a pile of omega blobs!
	create_omega_blobs(firing_segnum, firing_pos, goal_pos, parent_objp);

}
#endif

static inline int is_laser_weapon_type(enum weapon_type_t weapon_type)
{
#if defined(DXX_BUILD_DESCENT_II)
	if (weapon_type == LASER_ID_L5 || weapon_type == LASER_ID_L6)
		return 1;
#endif
	return (weapon_type == LASER_ID_L1 || weapon_type == LASER_ID_L2 || weapon_type == LASER_ID_L3 || weapon_type == LASER_ID_L4);
}

// ---------------------------------------------------------------------------------
// Initializes a laser after Fire is pressed
//	Returns object number.
objptridx_t Laser_create_new(const vms_vector &direction, const vms_vector &position, segnum_t segnum, const vobjptridx_t parent, enum weapon_type_t weapon_type, int make_sound )
{
	fix parent_speed, weapon_speed;
	fix volume;
	fix laser_length=0;

	Assert( weapon_type < N_weapon_types );

	switch(weapon_type)
	{
		case LASER_ID_L1:
		case LASER_ID_L2:
		case LASER_ID_L3:
		case LASER_ID_L4:
		case CLS1_DRONE_FIRE:
		case CONTROLCEN_WEAPON_NUM:
		case CONCUSSION_ID:
		case FLARE_ID:
		case CLS2_DRONE_LASER:
		case VULCAN_ID:
#if defined(DXX_BUILD_DESCENT_II)
		case SPREADFIRE_ID:
#endif
		case PLASMA_ID:
		case FUSION_ID:
		case HOMING_ID:
		case PROXIMITY_ID:
		case SMART_ID:
		case MEGA_ID:

		case PLAYER_SMART_HOMING_ID:
#if defined(DXX_BUILD_DESCENT_I)
		case SPREADFIRE_ID:
#endif
		case SUPER_MECH_MISS:
		case REGULAR_MECH_MISS:
		case SILENT_SPREADFIRE_ID:
		case MEDIUM_LIFTER_LASER:
		case SMALL_HULK_FIRE:
		case HEAVY_DRILLER_PLASMA:
		case SPIDER_ROBOT_FIRE:
		case ROBOT_MEGA_ID:
		case ROBOT_SMART_HOMING_ID:
#if defined(DXX_BUILD_DESCENT_II)
		case LASER_ID_L5:
		case LASER_ID_L6:

		case GAUSS_ID:
		case HELIX_ID:
		case PHOENIX_ID:
		case OMEGA_ID:

		case FLASH_ID:
		case GUIDEDMISS_ID:
		case SUPERPROX_ID:
		case MERCURY_ID:
		case EARTHSHAKER_ID:
		case SMELTER_PHOENIX_ID:

		case SMART_MINE_HOMING_ID:
		case ROBOT_SMART_MINE_HOMING_ID:
		case ROBOT_SUPERPROX_ID:
		case EARTHSHAKER_MEGA_ID:
		case ROBOT_EARTHSHAKER_ID:

		case PMINE_ID:

		case ROBOT_26_WEAPON_46_ID:
		case ROBOT_27_WEAPON_52_ID:
		case ROBOT_28_WEAPON_42_ID:
		case ROBOT_29_WEAPON_20_ID:
		case ROBOT_30_WEAPON_48_ID:
		case ROBOT_36_WEAPON_41_ID:
		case ROBOT_39_WEAPON_43_ID:
		case ROBOT_43_WEAPON_55_ID:
		case ROBOT_45_WEAPON_45_ID:
		case ROBOT_50_WEAPON_50_ID:
		case ROBOT_62_WEAPON_60_ID:
		case ROBOT_47_WEAPON_57_ID:
		case ROBOT_62_WEAPON_61_ID:
		case ROBOT_71_WEAPON_62_ID:
#endif
			break;
		default:
#ifdef NDEBUG
			break;
#else
			return object_none;
#endif
	}

	//	Don't let homing blobs make muzzle flash.
	if (parent->type == OBJ_ROBOT)
		do_muzzle_stuff(segnum, position);

	const objptridx_t obj = create_weapon_object(weapon_type,segnum,position);

	if ( obj == object_none ) {
		return obj;
	}

#if defined(DXX_BUILD_DESCENT_II)
	//	Do the special Omega Cannon stuff.  Then return on account of everything that follows does
	//	not apply to the Omega Cannon.
	if (weapon_type == OMEGA_ID) {
		// Create orientation matrix for tracking purposes.
		vm_vector_2_matrix( obj->orient, direction, &parent->orient.uvec ,nullptr);

		if (parent != Viewer && parent->type != OBJ_WEAPON) {
			// Muzzle flash
			if (Weapon_info[obj->id].flash_vclip > -1 )
				object_create_muzzle_flash( obj->segnum, obj->pos, Weapon_info[obj->id].flash_size, Weapon_info[obj->id].flash_vclip );
		}

		do_omega_stuff(parent, position, obj);

		return obj;
	}
#endif

	if (parent->type == OBJ_PLAYER) {
		if (weapon_type == FUSION_ID) {
			int	fusion_scale;
#if defined(DXX_BUILD_DESCENT_I)
			if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
				fusion_scale = 2;
			else
#endif
				fusion_scale = 4;

			if (Fusion_charge <= 0)
				obj->ctype.laser_info.multiplier = F1_0;
			else if (Fusion_charge <= F1_0*fusion_scale)
				obj->ctype.laser_info.multiplier = F1_0 + Fusion_charge/2;
			else
				obj->ctype.laser_info.multiplier = F1_0*fusion_scale;

#if defined(DXX_BUILD_DESCENT_I)
			//	Fusion damage was boosted by mk on 3/27 (for reg 1.1 release), but we only want it to apply to single player games.
			if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
				obj->ctype.laser_info.multiplier /= 2;
#endif
		}
		else if (is_laser_weapon_type(weapon_type) && (Players[get_player_id(parent)].flags & PLAYER_FLAGS_QUAD_LASERS))
			obj->ctype.laser_info.multiplier = F1_0*3/4;
#if defined(DXX_BUILD_DESCENT_II)
		else if (weapon_type == GUIDEDMISS_ID) {
			if (parent==Players[Player_num].objnum) {
				Guided_missile[Player_num]= obj;
				Guided_missile_sig[Player_num] = obj->signature;
				if (Newdemo_state==ND_STATE_RECORDING)
					newdemo_record_guided_start();
			}
		}
#endif
	}

	//	Make children of smart bomb bounce so if they hit a wall right away, they
	//	won't detonate.  The frame interval code will clear this bit after 1/2 second.
#if defined(DXX_BUILD_DESCENT_I)
	if ((weapon_type == PLAYER_SMART_HOMING_ID) || (weapon_type == ROBOT_SMART_HOMING_ID))
#elif defined(DXX_BUILD_DESCENT_II)
	if ((weapon_type == PLAYER_SMART_HOMING_ID) || (weapon_type == SMART_MINE_HOMING_ID) || (weapon_type == ROBOT_SMART_HOMING_ID) || (weapon_type == ROBOT_SMART_MINE_HOMING_ID) || (weapon_type == EARTHSHAKER_MEGA_ID))
#endif
		obj->mtype.phys_info.flags |= PF_BOUNCE;

	if (Weapon_info[weapon_type].render_type == WEAPON_RENDER_POLYMODEL)
		laser_length = Polygon_models[obj->rtype.pobj_info.model_num].rad * 2;

	if (weapon_type == FLARE_ID)
		obj->mtype.phys_info.flags |= PF_STICK;		//this obj sticks to walls

	obj->shields = Weapon_info[get_weapon_id(obj)].strength[Difficulty_level];

	// Fill in laser-specific data

	obj->lifeleft							= Weapon_info[get_weapon_id(obj)].lifetime;
	obj->ctype.laser_info.parent_type		= parent->type;
	obj->ctype.laser_info.parent_signature = parent->signature;
	obj->ctype.laser_info.parent_num			= parent;

	//	Assign parent type to highest level creator.  This propagates parent type down from
	//	the original creator through weapons which create children of their own (ie, smart missile)
	if (parent->type == OBJ_WEAPON) {
		auto highest_parent = parent;
		int	count;

		count = 0;
		while ((count++ < 10) && (highest_parent->type == OBJ_WEAPON)) {
			auto next_parent = highest_parent->ctype.laser_info.parent_num;
			if (Objects[next_parent].signature != highest_parent->ctype.laser_info.parent_signature)
				break;	//	Probably means parent was killed.  Just continue.

			if (next_parent == highest_parent) {
				Int3();	//	Hmm, object is parent of itself.  This would seem to be bad, no?
				break;
			}

			highest_parent = vobjptridx(next_parent);

			obj->ctype.laser_info.parent_num			= highest_parent;
			obj->ctype.laser_info.parent_type = highest_parent->type;
			obj->ctype.laser_info.parent_signature = highest_parent->signature;
		}
	}

	// Create orientation matrix so we can look from this pov
	//	Homing missiles also need an orientation matrix so they know if they can make a turn.
	if ((obj->render_type == RT_POLYOBJ) || (Weapon_info[get_weapon_id(obj)].homing_flag))
		vm_vector_2_matrix(obj->orient, direction, &parent->orient.uvec, nullptr);

	if (( parent != Viewer ) && (parent->type != OBJ_WEAPON))	{
		// Muzzle flash
		if (Weapon_info[get_weapon_id(obj)].flash_vclip > -1 )
			object_create_muzzle_flash( obj->segnum, obj->pos, Weapon_info[get_weapon_id(obj)].flash_size, Weapon_info[get_weapon_id(obj)].flash_vclip );
	}

	volume = F1_0;
	if (Weapon_info[get_weapon_id(obj)].flash_sound > -1 )	{
		if (make_sound)	{
			if ( parent == (Viewer-Objects) )	{
				if (weapon_type == VULCAN_ID)	// Make your own vulcan gun  1/2 as loud.
					volume = F1_0 / 2;
				digi_play_sample( Weapon_info[get_weapon_id(obj)].flash_sound, volume );
			} else {
				digi_link_sound_to_pos( Weapon_info[get_weapon_id(obj)].flash_sound, obj->segnum, 0, obj->pos, 0, volume );
			}
		}
	}

	//	Fire the laser from the gun tip so that the back end of the laser bolt is at the gun tip.
	// Move 1 frame, so that the end-tip of the laser is touching the gun barrel.
	// This also jitters the laser a bit so that it doesn't alias.
	//	Don't do for weapons created by weapons.
#if defined(DXX_BUILD_DESCENT_I)
	if (parent->type != OBJ_WEAPON && (Weapon_info[weapon_type].render_type != WEAPON_RENDER_NONE) && (weapon_type != FLARE_ID))
#elif defined(DXX_BUILD_DESCENT_II)
	if (parent->type == OBJ_PLAYER && (Weapon_info[weapon_type].render_type != WEAPON_RENDER_NONE) && (weapon_type != FLARE_ID))
#endif
	{
	 	const auto end_pos = vm_vec_scale_add(obj->pos, direction, (laser_length/2) );
		auto end_segnum = find_point_seg(end_pos, obj->segnum);
		if (end_segnum != obj->segnum) {
			if (end_segnum != segment_none) {
				obj->pos = end_pos;
				obj_relink(obj, end_segnum);
			}
		} else
			obj->pos = end_pos;
	}

	//	Here's where to fix the problem with objects which are moving backwards imparting higher velocity to their weaponfire.
	//	Find out if moving backwards.
	if (is_proximity_bomb_or_smart_mine(weapon_type)) {
		parent_speed = vm_vec_mag_quick(parent->mtype.phys_info.velocity);
		if (vm_vec_dot(parent->mtype.phys_info.velocity, parent->orient.fvec) < 0)
			parent_speed = -parent_speed;
	} else
		parent_speed = 0;

	weapon_speed = Weapon_info[get_weapon_id(obj)].speed[Difficulty_level];
#if defined(DXX_BUILD_DESCENT_II)
	if (Weapon_info[get_weapon_id(obj)].speedvar != 128) {
		fix	randval;

		//	Get a scale factor between speedvar% and 1.0.
		randval = F1_0 - ((d_rand() * Weapon_info[obj->id].speedvar) >> 6);
		weapon_speed = fixmul(weapon_speed, randval);
	}
#endif

	//	Ugly hack (too bad we're on a deadline), for homing missiles dropped by smart bomb, start them out slower.
#if defined(DXX_BUILD_DESCENT_I)
	if ((get_weapon_id(obj) == PLAYER_SMART_HOMING_ID) || (get_weapon_id(obj) == ROBOT_SMART_HOMING_ID))
#elif defined(DXX_BUILD_DESCENT_II)
	if ((get_weapon_id(obj) == PLAYER_SMART_HOMING_ID) || (get_weapon_id(obj) == SMART_MINE_HOMING_ID) || (get_weapon_id(obj) == ROBOT_SMART_HOMING_ID) || (get_weapon_id(obj) == ROBOT_SMART_MINE_HOMING_ID) || (get_weapon_id(obj) == EARTHSHAKER_MEGA_ID))
#endif
		weapon_speed /= 4;

	if (Weapon_info[get_weapon_id(obj)].thrust != 0)
		weapon_speed /= 2;

	vm_vec_copy_scale(obj->mtype.phys_info.velocity, direction, weapon_speed + parent_speed );

	//	Set thrust
	if (Weapon_info[weapon_type].thrust != 0) {
		obj->mtype.phys_info.thrust = obj->mtype.phys_info.velocity;
		vm_vec_scale(obj->mtype.phys_info.thrust, fixdiv(Weapon_info[get_weapon_id(obj)].thrust, weapon_speed+parent_speed));
	}

	if ((obj->type == OBJ_WEAPON) && (get_weapon_id(obj) == FLARE_ID))
		obj->lifeleft += (d_rand()-16384) << 2;		//	add in -2..2 seconds

	return obj;
}

//	-----------------------------------------------------------------------------------------------------------
//	Calls Laser_create_new, but takes care of the segment and point computation for you.
objptridx_t Laser_create_new_easy(const vms_vector &direction, const vms_vector &position, const vobjptridx_t parent, enum weapon_type_t weapon_type, int make_sound )
{
	fvi_query	fq;
	fvi_info		hit_data;
	int			fate;

	//	Find segment containing laser fire position.  If the robot is straddling a segment, the position from
	//	which it fires may be in a different segment, which is bad news for find_vector_intersection.  So, cast
	//	a ray from the object center (whose segment we know) to the laser position.  Then, in the call to Laser_create_new
	//	use the data returned from this call to find_vector_intersection.
	//	Note that while find_vector_intersection is pretty slow, it is not terribly slow if the destination point is
	//	in the same segment as the source point.

	fq.p0						= &parent->pos;
	fq.startseg				= parent->segnum;
	fq.p1						= &position;
	fq.rad					= 0;
	fq.thisobjnum			= parent;
	fq.ignore_obj_list.first = nullptr;
	fq.flags					= FQ_TRANSWALL | FQ_CHECK_OBJS;		//what about trans walls???

	fate = find_vector_intersection(fq, hit_data);
	if (fate != HIT_NONE  || hit_data.hit_seg==segment_none) {
		return object_none;
	}

	return Laser_create_new( direction, hit_data.hit_pnt, hit_data.hit_seg, parent, weapon_type, make_sound );

}

int		Muzzle_queue_index = 0;

array<muzzle_info, MUZZLE_QUEUE_MAX> Muzzle_data;

//	-----------------------------------------------------------------------------------------------------------
//	Determine if two objects are on a line of sight.  If so, return true, else return false.
//	Calls fvi.
int object_to_object_visibility(const vcobjptridx_t obj1, const vcobjptr_t obj2, int trans_type)
{
	fvi_query	fq;
	fvi_info		hit_data;

	fq.p0						= &obj1->pos;
	fq.startseg				= obj1->segnum;
	fq.p1						= &obj2->pos;
	fq.rad					= 0x10;
	fq.thisobjnum			= obj1;
	fq.ignore_obj_list.first = nullptr;
	fq.flags					= trans_type;

	switch(const auto fate = find_vector_intersection(fq, hit_data))
	{
		case HIT_NONE:
			return 1;
		case HIT_WALL:
			return 0;
		default:
			con_printf(CON_VERBOSE, "object_to_object_visibility: fate=%u for object %hu{%hu/%i,%i,%i} to {%i,%i,%i}", fate, static_cast<vcobjptridx_t::integral_type>(obj1), obj1->segnum, obj1->pos.x, obj1->pos.y, obj1->pos.z, obj2->pos.x, obj2->pos.y, obj2->pos.z);
		// Int3();		//	Contact Mike: Oops, what happened?  What is fate?
						// 2 = hit object (impossible), 3 = bad starting point (bad)
			break;
	}
	return 0;
}


//	-----------------------------------------------------------------------------------------------------------
//	Return true if weapon *tracker is able to track object Objects[track_goal], else return false.
//	In order for the object to be trackable, it must be within a reasonable turning radius for the missile
//	and it must not be obstructed by a wall.
static int object_is_trackable(const objptridx_t objp, const vobjptridx_t tracker, fix *dot)
{
	if (objp == object_none)
		return 0;
	if (Game_mode & GM_MULTI_COOP)
		return 0;
	//	Don't track player if he's cloaked.
	if ((objp == Players[Player_num].objnum) && (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED))
		return 0;

	//	Can't track AI object if he's cloaked.
	if (objp->type == OBJ_ROBOT) {
		if (objp->ctype.ai_info.CLOAKED)
			return 0;
#if defined(DXX_BUILD_DESCENT_II)
		//	Your missiles don't track your escort.
		if (Robot_info[get_robot_id(objp)].companion)
			if (tracker->ctype.laser_info.parent_type == OBJ_PLAYER)
				return 0;
#endif
	}
	const auto vector_to_goal = vm_vec_normalized_quick(vm_vec_sub(objp->pos, tracker->pos));
	*dot = vm_vec_dot(vector_to_goal, tracker->orient.fvec);

	if (*dot >= HOMING_MIN_TRACKABLE_DOT) {
		int	rval;
		//	dot is in legal range, now see if object is visible
		rval =  object_to_object_visibility(tracker, objp, FQ_TRANSWALL);
		return rval;
	} else {
		return 0;
	}
}

//	--------------------------------------------------------------------------------------------
static objptridx_t call_find_homing_object_complete(const vms_vector &curpos, const vobjptridx_t tracker)
{
	if (Game_mode & GM_MULTI) {
		if (tracker->ctype.laser_info.parent_type == OBJ_PLAYER) {
			//	It's fired by a player, so if robots present, track robot, else track player.
			if (Game_mode & GM_MULTI_COOP)
				return find_homing_object_complete( curpos, tracker, OBJ_ROBOT, -1);
			else
				return find_homing_object_complete( curpos, tracker, OBJ_PLAYER, OBJ_ROBOT);
		} else {
			int	goal2_type = -1;
#if defined(DXX_BUILD_DESCENT_II)
			if (cheats.robotskillrobots)
				goal2_type = OBJ_ROBOT;
#endif
			Assert(tracker->ctype.laser_info.parent_type == OBJ_ROBOT);
			return find_homing_object_complete(curpos, tracker, OBJ_PLAYER, goal2_type);
		}
	} else
		return find_homing_object_complete( curpos, tracker, OBJ_ROBOT, -1);
}

//	--------------------------------------------------------------------------------------------
//	Find object to home in on.
//	Scan list of objects rendered last frame, find one that satisfies function of nearness to center and distance.
static objptridx_t find_homing_object(const vms_vector &curpos, const vobjptridx_t tracker)
{
	//	Contact Mike: This is a bad and stupid thing.  Who called this routine with an illegal laser type??
#if defined(DXX_BUILD_DESCENT_II)
	if (tracker->id != OMEGA_ID)
#endif
		Assert(Weapon_info[get_weapon_id(tracker)].homing_flag);

	//	Find an object to track based on game mode (eg, whether in network play) and who fired it.

		return call_find_homing_object_complete(curpos, tracker);
}

//	--------------------------------------------------------------------------------------------
//	Find object to home in on.
//	Scan list of objects rendered last frame, find one that satisfies function of nearness to center and distance.
//	Can track two kinds of objects.  If you are only interested in one type, set track_obj_type2 to NULL
//	Always track proximity bombs.  --MK, 06/14/95
//	Make homing objects not track parent's prox bombs.
objptridx_t find_homing_object_complete(const vms_vector &curpos, const vobjptridx_t tracker, int track_obj_type1, int track_obj_type2)
{
	fix	max_dot = -F1_0*2;

#if defined(DXX_BUILD_DESCENT_II)
	if (tracker->id != OMEGA_ID)
	//	Contact Mike: This is a bad and stupid thing.  Who called this routine with an illegal laser type??
#endif
	{
		if (!Weapon_info[get_weapon_id(tracker)].homing_flag)
			throw std::logic_error("tracking without homing_flag");
	}

	const fix64 HOMING_MAX_TRACKABLE_DIST = F1_0*250;
	vm_distance_squared max_trackable_dist{HOMING_MAX_TRACKABLE_DIST * HOMING_MAX_TRACKABLE_DIST};
	fix min_trackable_dot = HOMING_MAX_TRACKABLE_DOT;

#if defined(DXX_BUILD_DESCENT_II)
	if (tracker->id == OMEGA_ID) {
		max_trackable_dist = OMEGA_MAX_TRACKABLE_DIST * OMEGA_MAX_TRACKABLE_DIST;
		min_trackable_dot = OMEGA_MIN_TRACKABLE_DOT;
	}
#endif

	objptridx_t	best_objnum = object_none;
	range_for (const auto objnum, highest_valid(Objects))
	{
		int			is_proximity = 0;
		fix			dot;
		auto curobjp = vobjptridx(objnum);

		if ((curobjp->type != track_obj_type1) && (curobjp->type != track_obj_type2))
		{
#if defined(DXX_BUILD_DESCENT_II)
			if ((curobjp->type == OBJ_WEAPON) && (is_proximity_bomb_or_smart_mine(get_weapon_id(curobjp)))) {
				if (curobjp->ctype.laser_info.parent_signature != tracker->ctype.laser_info.parent_signature)
					is_proximity = 1;
				else
					continue;
			} else
#endif
				continue;
		}

		if (objnum == tracker->ctype.laser_info.parent_num) // Don't track shooter
			continue;

		//	Don't track cloaked players.
		if (curobjp->type == OBJ_PLAYER)
		{
			if (Players[get_player_id(curobjp)].flags & PLAYER_FLAGS_CLOAKED)
				continue;
			// Don't track teammates in team games
			if ((Game_mode & GM_TEAM) && (Objects[tracker->ctype.laser_info.parent_num].type == OBJ_PLAYER) && (get_team(get_player_id(curobjp)) == get_team(get_player_id(&Objects[tracker->ctype.laser_info.parent_num]))))
				continue;
		}

		//	Can't track AI object if he's cloaked.
		if (curobjp->type == OBJ_ROBOT) {
			if (curobjp->ctype.ai_info.CLOAKED)
				continue;

#if defined(DXX_BUILD_DESCENT_II)
			//	Your missiles don't track your escort.
			if (Robot_info[curobjp->id].companion)
				if (tracker->ctype.laser_info.parent_type == OBJ_PLAYER)
					continue;
#endif
		}

		auto vec_to_curobj = vm_vec_sub(curobjp->pos, curpos);
		auto dist = vm_vec_mag2(vec_to_curobj);

		if (dist < max_trackable_dist) {
			vm_vec_normalize(vec_to_curobj);
			dot = vm_vec_dot(vec_to_curobj, tracker->orient.fvec);
			if (is_proximity)
				dot = ((dot << 3) + dot) >> 3;		//	I suspect Watcom would be too stupid to figure out the obvious...

			if (dot > min_trackable_dot) {
				if (dot > max_dot) {
					if (object_to_object_visibility(tracker, curobjp, FQ_TRANSWALL)) {
						max_dot = dot;
						best_objnum = curobjp;
					}
				}
			}
		}

	}
	return best_objnum;
}

//	------------------------------------------------------------------------------------------------------------
//	See if legal to keep tracking currently tracked object.  If not, see if another object is trackable.  If not, return -1,
//	else return object number of tracking object.
//	Computes and returns a fairly precise dot product.
static objptridx_t track_track_goal(const objptridx_t track_goal, const vobjptridx_t tracker, fix *dot)
{
	if (object_is_trackable(track_goal, tracker, dot)) {
		return track_goal;
	} else if ((((tracker) ^ d_tick_count) % 4) == 0)
	{
		//	If player fired missile, then search for an object, if not, then give up.
		if (Objects[tracker->ctype.laser_info.parent_num].type == OBJ_PLAYER) {
			int	goal_type;

			if (track_goal == object_none)
			{
				if (Game_mode & GM_MULTI)
				{
					if (Game_mode & GM_MULTI_COOP)
						return find_homing_object_complete( tracker->pos, tracker, OBJ_ROBOT, -1);
					else if (Game_mode & GM_MULTI_ROBOTS)		//	Not cooperative, if robots, track either robot or player
						return find_homing_object_complete( tracker->pos, tracker, OBJ_PLAYER, OBJ_ROBOT);
					else		//	Not cooperative and no robots, track only a player
						return find_homing_object_complete( tracker->pos, tracker, OBJ_PLAYER, -1);
				}
				else
					return find_homing_object_complete(tracker->pos, tracker, OBJ_PLAYER, OBJ_ROBOT);
			}
			else
			{
				goal_type = Objects[tracker->ctype.laser_info.track_goal].type;
				if ((goal_type == OBJ_PLAYER) || (goal_type == OBJ_ROBOT))
					return find_homing_object_complete(tracker->pos, tracker, goal_type, -1);
				else
					return object_none;
			}
		}
		else {
			int	goal_type, goal2_type = -1;

#if defined(DXX_BUILD_DESCENT_II)
			if (cheats.robotskillrobots)
				goal2_type = OBJ_ROBOT;
#endif

			if (track_goal == object_none)
				return find_homing_object_complete(tracker->pos, tracker, OBJ_PLAYER, goal2_type);
			else {
				goal_type = Objects[tracker->ctype.laser_info.track_goal].type;
				return find_homing_object_complete(tracker->pos, tracker, goal_type, goal2_type);
			}
		}
	}

	return object_none;
}

//-------------- Initializes a laser after Fire is pressed -----------------

static objptridx_t Laser_player_fire_spread_delay(const vobjptridx_t obj, enum weapon_type_t laser_type, int gun_num, fix spreadr, fix spreadu, fix delay_time, int make_sound, vms_vector shot_orientation)
{
	int			Fate;
	vms_vector	LaserDir;
	fvi_query	fq;
	fvi_info		hit_data;
	vms_vector	*pnt;

#if defined(DXX_BUILD_DESCENT_II)
	create_awareness_event(obj, player_awareness_type_t::PA_WEAPON_WALL_COLLISION);
#endif

	// Find the initial position of the laser
	pnt = &Player_ship->gun_points[gun_num];

	vms_matrix m = vm_transposed_matrix(obj->orient);
	const auto gun_point = vm_vec_rotate(*pnt,m);

	auto LaserPos = vm_vec_add(obj->pos,gun_point);

	//	If supposed to fire at a delayed time (delay_time), then move this point backwards.
	if (delay_time)
		vm_vec_scale_add2(LaserPos, shot_orientation, -fixmul(delay_time, Weapon_info[laser_type].speed[Difficulty_level]));

//	do_muzzle_stuff(obj, &Pos);

	//--------------- Find LaserPos and LaserSeg ------------------
	fq.p0						= &obj->pos;
	fq.startseg				= obj->segnum;
	fq.p1						= &LaserPos;
	fq.rad					= 0x10;
	fq.thisobjnum			= obj;
	fq.ignore_obj_list.first = nullptr;
#if defined(DXX_BUILD_DESCENT_I)
	fq.flags					= FQ_CHECK_OBJS;
#elif defined(DXX_BUILD_DESCENT_II)
	fq.flags					= FQ_CHECK_OBJS | FQ_IGNORE_POWERUPS;
#endif

	Fate = find_vector_intersection(fq, hit_data);

	auto LaserSeg = hit_data.hit_seg;

	if (LaserSeg == segment_none)		//some sort of annoying error
		return object_none;

	//SORT OF HACK... IF ABOVE WAS CORRECT THIS WOULDNT BE NECESSARY.
	if ( vm_vec_dist_quick(LaserPos, obj->pos) > 0x50000 )
		return object_none;

	if (Fate==HIT_WALL) {
		return object_none;
	}

	if (Fate==HIT_OBJECT) {
//		if ( Objects[hit_data.hit_object].type == OBJ_ROBOT )
//			Objects[hit_data.hit_object].flags |= OF_SHOULD_BE_DEAD;
//		if ( Objects[hit_data.hit_object].type != OBJ_POWERUP )
//			return;
	//as of 12/6/94, we don't care if the laser is stuck in an object. We
	//just fire away normally
	}

	//	Now, make laser spread out.
	LaserDir = shot_orientation;
	if ((spreadr != 0) || (spreadu != 0)) {
		vm_vec_scale_add2(LaserDir, obj->orient.rvec, spreadr);
		vm_vec_scale_add2(LaserDir, obj->orient.uvec, spreadu);
	}

	auto objnum = Laser_create_new( LaserDir, LaserPos, LaserSeg, obj, laser_type, make_sound );

	if (objnum == object_none)
		return objnum;

#if defined(DXX_BUILD_DESCENT_II)
	//	Omega cannon is a hack, not surprisingly.  Don't want to do the rest of this stuff.
	if (laser_type == OMEGA_ID)
		return objnum;

	if (laser_type==GUIDEDMISS_ID && Multi_is_guided) {
		Guided_missile[obj->id] = objnum;
	}

	Multi_is_guided=0;

	if (laser_type == CONCUSSION_ID ||
			 laser_type == HOMING_ID ||
			 laser_type == SMART_ID ||
			 laser_type == MEGA_ID ||
			 laser_type == FLASH_ID ||
			 //laser_type == GUIDEDMISS_ID ||
			 //laser_type == SUPERPROX_ID ||
			 laser_type == MERCURY_ID ||
			 laser_type == EARTHSHAKER_ID)
	{
		const auto need_new_missile_viewer = [obj]{
			if (!Missile_viewer)
				return true;
			if (Missile_viewer->type != OBJ_WEAPON)
				return true;
			if (Missile_viewer->signature != Missile_viewer_sig)
				return true;
			if (obj->id == Player_num && Missile_viewer->ctype.laser_info.parent_num != Players[Player_num].objnum)
				/* New missile fired-by local player &&
				 * currently viewing missile not-fired-by local player
				 */
				return true;
			return false;
		};
		const auto can_view_missile = [obj]{
			if (obj->id == Player_num)
				return true;
			if (Game_mode & GM_MULTI_COOP)
				return true;
			if (Game_mode & GM_TEAM)
				return get_team(Player_num) == get_team(obj->id);
			return false;
		};
		if (need_new_missile_viewer() && can_view_missile())
		{
			Missile_viewer = objnum;
			Missile_viewer_sig = objnum->signature;
		}
	}
#endif

	//	If this weapon is supposed to be silent, set that bit!
	if (!make_sound)
		objnum->flags |= OF_SILENT;

	//	If the object firing the laser is the player, then indicate the laser object so robots can dodge.
	//	New by MK on 6/8/95, don't let robots evade proximity bombs, thereby decreasing uselessness of bombs.
	if (obj == ConsoleObject)
#if defined(DXX_BUILD_DESCENT_II)
		if (objnum->id != PROXIMITY_ID && objnum->id != SUPERPROX_ID)
#endif
		Player_fired_laser_this_frame = objnum;

	if (Weapon_info[laser_type].homing_flag) {
		if (obj == ConsoleObject)
		{
			objnum->ctype.laser_info.track_goal = find_homing_object(LaserPos, objnum);
			Network_laser_track = objnum->ctype.laser_info.track_goal;
		}
		else // Some other player shot the homing thing
		{
			Assert(Game_mode & GM_MULTI);
			objnum->ctype.laser_info.track_goal = Network_laser_track;
		}
		objnum->ctype.laser_info.track_turn_time = HOMING_TURN_TIME;
	}

	return objnum;
}

//	-----------------------------------------------------------------------------------------------------------
static objptridx_t Laser_player_fire_spread(const vobjptridx_t obj, enum weapon_type_t laser_type, int gun_num, fix spreadr, fix spreadu, int make_sound, vms_vector shot_orientation)
{
	return Laser_player_fire_spread_delay(obj, laser_type, gun_num, spreadr, spreadu, 0, make_sound, shot_orientation);
}


//	-----------------------------------------------------------------------------------------------------------
objptridx_t Laser_player_fire(const vobjptridx_t obj, enum weapon_type_t laser_type, int gun_num, int make_sound, vms_vector shot_orientation)
{
	return Laser_player_fire_spread(obj, laser_type, gun_num, 0, 0, make_sound, shot_orientation);
}

//	-----------------------------------------------------------------------------------------------------------
void Flare_create(const vobjptridx_t obj)
{
	fix	energy_usage;

	energy_usage = Weapon_info[FLARE_ID].energy_usage;

	if (Difficulty_level < 2)
		energy_usage = fixmul(energy_usage, i2f(Difficulty_level+2)/4);

//	MK, 11/04/95: Allowed to fire flare even if no energy.
// -- 	if (Players[Player_num].energy >= energy_usage)
#if defined(DXX_BUILD_DESCENT_I)
	if (Players[Player_num].energy > 0)
#endif
	{
		Players[Player_num].energy -= energy_usage;

		if (Players[Player_num].energy <= 0) {
			Players[Player_num].energy = 0;
#if defined(DXX_BUILD_DESCENT_I)
			auto_select_weapon(0);
#endif
		}

		Laser_player_fire( obj, FLARE_ID, 6, 1, Objects[Players[Player_num].objnum].orient.fvec);

		if (Game_mode & GM_MULTI)
			multi_send_fire(FLARE_ADJUST, 0, 0, 1, object_none, object_none);
	}

}

#if defined(DXX_BUILD_DESCENT_I)
#define	HOMING_MISSILE_SCALE	8
#elif defined(DXX_BUILD_DESCENT_II)
#define	HOMING_MISSILE_SCALE	16
#endif

//--------------------------------------------------------------------
//	Set object *objp's orientation to (or towards if I'm ambitious) its velocity.
static void homing_missile_turn_towards_velocity(const vobjptr_t objp, const vms_vector &norm_vel)
{
	auto new_fvec = norm_vel;
	vm_vec_scale(new_fvec, FrameTime * HOMING_MISSILE_SCALE);
	vm_vec_add2(new_fvec, objp->orient.fvec);
	vm_vec_normalize_quick(new_fvec);

//	if ((norm_vel->x == 0) && (norm_vel->y == 0) && (norm_vel->z == 0))
//		return;

	vm_vector_2_matrix(objp->orient, new_fvec, nullptr, nullptr);
}


//-------------------------------------------------------------------------------------------
//sequence this laser object for this _frame_ (underscores added here to aid MK in his searching!)
void Laser_do_weapon_sequence(const vobjptridx_t obj)
{
	Assert(obj->control_type == CT_WEAPON);

	if (obj->lifeleft < 0 ) {		// We died of old age
		obj->flags |= OF_SHOULD_BE_DEAD;
		if ( Weapon_info[get_weapon_id(obj)].damage_radius )
			explode_badass_weapon(obj, obj->pos);
		return;
	}

#if defined(DXX_BUILD_DESCENT_II)
	if (omega_cleanup(obj))
		return;
#endif

	//delete weapons that are not moving
	if (	!((d_tick_count ^ obj->signature.get()) & 3) &&
			(get_weapon_id(obj) != FLARE_ID) &&
			(Weapon_info[get_weapon_id(obj)].speed[Difficulty_level] > 0) &&
			(vm_vec_mag_quick(obj->mtype.phys_info.velocity) < F2_0)) {
		obj_delete(obj);
		return;
	}

	if ( get_weapon_id(obj) == FUSION_ID ) {		//always set fusion weapon to max vel

		vm_vec_normalize_quick(obj->mtype.phys_info.velocity);

		vm_vec_scale(obj->mtype.phys_info.velocity, Weapon_info[get_weapon_id(obj)].speed[Difficulty_level]);
	}

	//	For homing missiles, turn towards target. (unless it's the guided missile)
#if defined(DXX_BUILD_DESCENT_I)
	if (Weapon_info[get_weapon_id(obj)].homing_flag)
#elif defined(DXX_BUILD_DESCENT_II)
	if (Weapon_info[get_weapon_id(obj)].homing_flag && !(get_weapon_id(obj)==GUIDEDMISS_ID && obj->ctype.laser_info.parent_type==OBJ_PLAYER && obj==Guided_missile[get_player_id(&Objects[obj->ctype.laser_info.parent_num])] && obj->signature==Guided_missile[Objects[obj->ctype.laser_info.parent_num].id]->signature))
#endif
	{
		vms_vector		vector_to_object, temp_vec;
		fix				dot=F1_0;
		fix				speed, max_speed;

		//	For first 125ms of life, missile flies straight.
		if (obj->ctype.laser_info.creation_time + HOMING_FLY_STRAIGHT_TIME < GameTime64) {

			//	If it's time to do tracking, then it's time to grow up, stop bouncing and start exploding!.
#if defined(DXX_BUILD_DESCENT_I)
			if ((get_weapon_id(obj) == ROBOT_SMART_HOMING_ID) || (get_weapon_id(obj) == PLAYER_SMART_HOMING_ID))
#elif defined(DXX_BUILD_DESCENT_II)
			if ((get_weapon_id(obj) == ROBOT_SMART_MINE_HOMING_ID) || (get_weapon_id(obj) == ROBOT_SMART_HOMING_ID) || (get_weapon_id(obj) == SMART_MINE_HOMING_ID) || (get_weapon_id(obj) == PLAYER_SMART_HOMING_ID) || (get_weapon_id(obj) == EARTHSHAKER_MEGA_ID))
#endif
			{
				obj->mtype.phys_info.flags &= ~PF_BOUNCE;
			}

			//	Make sure the object we are tracking is still trackable.
			objptridx_t obj_track_goal = object_none;
			if (obj->ctype.laser_info.track_goal != object_none)
				obj_track_goal = obj->ctype.laser_info.track_goal;
			auto track_goal = track_track_goal(obj_track_goal, obj, &dot);

			if (track_goal == Players[Player_num].objnum) {
				fix	dist_to_player;

				dist_to_player = vm_vec_dist_quick(obj->pos, track_goal->pos);
				if ((dist_to_player < Players[Player_num].homing_object_dist) || (Players[Player_num].homing_object_dist < 0))
					Players[Player_num].homing_object_dist = dist_to_player;

			}

			if (track_goal != object_none) {
#ifdef NEWHOMER
				// See if enough time (see HOMING_TURN_TIME) passed and if yes, allow a turn. If not, fly straight.
				if (obj->ctype.laser_info.track_turn_time >= HOMING_TURN_TIME)
				{
					vm_vec_sub(vector_to_object, track_goal->pos, obj->pos);
					obj->ctype.laser_info.track_turn_time -= HOMING_TURN_TIME;
				}
				else
				{
					const auto straight = vm_vec_add(obj->mtype.phys_info.velocity, obj->pos);
					vm_vec_sub(vector_to_object, straight, obj->pos);
				}
				obj->ctype.laser_info.track_turn_time += FrameTime;

				// Scale vector to object to current FrameTime if we run really low
				if (FrameTime > HOMING_TURN_TIME)
					vm_vec_scale(vector_to_object, F1_0/((float)HOMING_TURN_TIME/FrameTime));
#else
				vm_vec_sub(vector_to_object, Objects[track_goal].pos, obj->pos);
#endif
				vm_vec_normalize_quick(vector_to_object);
				temp_vec = obj->mtype.phys_info.velocity;
				speed = vm_vec_normalize_quick(temp_vec);
				max_speed = Weapon_info[get_weapon_id(obj)].speed[Difficulty_level];
				if (speed+F1_0 < max_speed) {
					speed += fixmul(max_speed, FrameTime/2);
					if (speed > max_speed)
						speed = max_speed;
				}

				dot = vm_vec_dot(temp_vec, vector_to_object);

				vm_vec_add2(temp_vec, vector_to_object);
				//	The boss' smart children track better...
				if (Weapon_info[get_weapon_id(obj)].render_type != WEAPON_RENDER_POLYMODEL)
					vm_vec_add2(temp_vec, vector_to_object);
				vm_vec_normalize_quick(temp_vec);
				vm_vec_scale(temp_vec, speed);
				obj->mtype.phys_info.velocity = temp_vec;

				//	Subtract off life proportional to amount turned.
				//	For hardest turn, it will lose 2 seconds per second.
				{
					fix	lifelost, absdot;

					absdot = abs(F1_0 - dot);

					lifelost = fixmul(absdot*32, FrameTime);
					obj->lifeleft -= lifelost;
				}

				//	Only polygon objects have visible orientation, so only they should turn.
				if (Weapon_info[get_weapon_id(obj)].render_type == WEAPON_RENDER_POLYMODEL)
					homing_missile_turn_towards_velocity(obj, temp_vec);		//	temp_vec is normalized velocity.
			}
		}
	}

	//	Make sure weapon is not moving faster than allowed speed.
#if defined(DXX_BUILD_DESCENT_I)
	if (Weapon_info[get_weapon_id(obj)].thrust != 0)
#endif
	{
		fix	weapon_speed;

		weapon_speed = vm_vec_mag_quick(obj->mtype.phys_info.velocity);
		if (weapon_speed > Weapon_info[get_weapon_id(obj)].speed[Difficulty_level]) {
			//	Only slow down if not allowed to move.  Makes sense, huh?  Allows proxbombs to get moved by physics force. --MK, 2/13/96
#if defined(DXX_BUILD_DESCENT_II)
			if (Weapon_info[obj->id].speed[Difficulty_level])
#endif
			{
				fix	scale_factor;

				scale_factor = fixdiv(Weapon_info[get_weapon_id(obj)].speed[Difficulty_level], weapon_speed);
				vm_vec_scale(obj->mtype.phys_info.velocity, scale_factor);
			}
		}
	}
}

fix64	Last_laser_fired_time = 0;

static inline int sufficient_energy(int energy_used, fix energy)
{
	return !energy_used || (energy >= energy_used);
}

static inline int sufficient_ammo(int ammo_used, int uses_vulcan_ammo, ushort vulcan_ammo)
{
	return !ammo_used || (!uses_vulcan_ammo || vulcan_ammo >= ammo_used);
}

//	--------------------------------------------------------------------------------------------------
// Assumption: This is only called by the actual console player, not for network players

int do_laser_firing_player(void)
{
	player	*plp = &Players[Player_num];
	fix		energy_used;
	int		ammo_used;
	int		weapon_index;
	int		rval = 0;
	int 		nfires = 1;
	static int Spreadfire_toggle=0;
#if defined(DXX_BUILD_DESCENT_II)
	static int Helix_orientation = 0;
#endif

	if (Player_is_dead)
		return 0;

	weapon_index = Primary_weapon_to_weapon_info[Primary_weapon];
	energy_used = Weapon_info[weapon_index].energy_usage;

	if (Difficulty_level < 2)
		energy_used = fixmul(energy_used, i2f(Difficulty_level+2)/4);

	ammo_used = Weapon_info[weapon_index].ammo_usage;

	int uses_vulcan_ammo = weapon_index_uses_vulcan_ammo(Primary_weapon);

#if defined(DXX_BUILD_DESCENT_II)
	if (Primary_weapon == OMEGA_INDEX)
		energy_used = 0;	//	Omega consumes energy when recharging, not when firing.
	//	MK, 01/26/96, Helix use 2x energy in multiplayer.  bitmaps.tbl parm should have been reduced for single player.
	if (weapon_index == HELIX_INDEX)
		if (Game_mode & GM_MULTI)
			energy_used *= 2;

	if	(!(sufficient_energy(energy_used, plp->energy) && sufficient_ammo(ammo_used, uses_vulcan_ammo, plp->vulcan_ammo)))
		auto_select_weapon(0);		//	Make sure the player can fire from this weapon.
#endif

	while (Next_laser_fire_time <= GameTime64) {
		if	(sufficient_energy(energy_used, plp->energy) && sufficient_ammo(ammo_used, uses_vulcan_ammo, plp->vulcan_ammo)) {
			int	laser_level, flags, fire_frame_overhead = 0;

			if (GameTime64 - Next_laser_fire_time <= FrameTime) // if firing is prolonged by FrameTime overhead, let's try to fix that.
				fire_frame_overhead = GameTime64 - Next_laser_fire_time;

                        Last_laser_fired_time = GameTime64;

			if (!cheats.rapidfire)
				Next_laser_fire_time = GameTime64 + Weapon_info[weapon_index].fire_wait - fire_frame_overhead;
			else
				Next_laser_fire_time = GameTime64 + (F1_0/25) - fire_frame_overhead;

			laser_level = Players[Player_num].laser_level;

			flags = 0;

			if (Primary_weapon == SPREADFIRE_INDEX) {
				if (Spreadfire_toggle)
					flags |= LASER_SPREADFIRE_TOGGLED;
				Spreadfire_toggle = !Spreadfire_toggle;
			}

#if defined(DXX_BUILD_DESCENT_II)
			if (Primary_weapon == HELIX_INDEX) {
				Helix_orientation++;
				flags |= ((Helix_orientation & LASER_HELIX_MASK) << LASER_HELIX_SHIFT);
			}
#endif

			if (Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS)
				flags |= LASER_QUAD;

			rval += do_laser_firing(vobjptridx(Players[Player_num].objnum), Primary_weapon, laser_level, flags, nfires, Objects[Players[Player_num].objnum].orient.fvec);

			plp->energy -= (energy_used * rval) / Weapon_info[weapon_index].fire_count;
			if (plp->energy < 0)
				plp->energy = 0;

			if (uses_vulcan_ammo) {
				if (ammo_used > plp->vulcan_ammo)
					plp->vulcan_ammo = 0;
				else
					plp->vulcan_ammo -= ammo_used;
			}

			auto_select_weapon(0);		//	Make sure the player can fire from this weapon.

		} else {
#if defined(DXX_BUILD_DESCENT_II)
			auto_select_weapon(0);		//	Make sure the player can fire from this weapon.
			Next_laser_fire_time = GameTime64;	//	Prevents shots-to-fire from building up.
#endif
			break;	//	Couldn't fire weapon, so abort.
		}
	}

	Global_laser_firing_count = 0;

	return rval;
}

//	--------------------------------------------------------------------------------------------------
//	Object "objnum" fires weapon "weapon_num" of level "level".  (Right now (9/24/94) level is used only for type 0 laser.
//	Flags are the player flags.  For network mode, set to 0.
//	It is assumed that this is a player object (as in multiplayer), and therefore the gun positions are known.
//	Returns number of times a weapon was fired.  This is typically 1, but might be more for low frame rates.
//	More than one shot is fired with a pseudo-delay so that players on slow machines can fire (for themselves
//	or other players) often enough for things like the vulcan cannon.
int do_laser_firing(vobjptridx_t objp, int weapon_num, int level, int flags, int nfires, vms_vector shot_orientation)
{
	switch (weapon_num) {
		case LASER_INDEX: {
			enum weapon_type_t weapon_type;

			switch(level)
			{
				case LASER_LEVEL_1:
					weapon_type = LASER_ID_L1;
					break;
				case LASER_LEVEL_2:
					weapon_type = LASER_ID_L2;
					break;
				case LASER_LEVEL_3:
					weapon_type = LASER_ID_L3;
					break;
				case LASER_LEVEL_4:
					weapon_type = LASER_ID_L4;
					break;
#if defined(DXX_BUILD_DESCENT_II)
				case LASER_LEVEL_5:
					weapon_type = LASER_ID_L5;
					break;
				case LASER_LEVEL_6:
					weapon_type = LASER_ID_L6;
					break;
#endif
				default:
					Assert(0);
					return nfires;
			}
			Laser_player_fire( objp, weapon_type, 0, 1, shot_orientation);
			Laser_player_fire( objp, weapon_type, 1, 0, shot_orientation);

			if (flags & LASER_QUAD) {
				//	hideous system to make quad laser 1.5x powerful as normal laser, make every other quad laser bolt harmless
				Laser_player_fire( objp, weapon_type, 2, 0, shot_orientation);
				Laser_player_fire( objp, weapon_type, 3, 0, shot_orientation);
			}
			break;
		}
		case VULCAN_INDEX: {
			//	Only make sound for 1/4 of vulcan bullets.
			int	make_sound = 1;
			//if (d_rand() > 24576)
			//	make_sound = 1;
			Laser_player_fire_spread( objp, VULCAN_ID, 6, d_rand()/8 - 32767/16, d_rand()/8 - 32767/16, make_sound, shot_orientation);
			if (nfires > 1) {
				Laser_player_fire_spread( objp, VULCAN_ID, 6, d_rand()/8 - 32767/16, d_rand()/8 - 32767/16, 0, shot_orientation);
				if (nfires > 2) {
					Laser_player_fire_spread( objp, VULCAN_ID, 6, d_rand()/8 - 32767/16, d_rand()/8 - 32767/16, 0, shot_orientation);
				}
			}
			break;
		}
		case SPREADFIRE_INDEX:
			if (flags & LASER_SPREADFIRE_TOGGLED) {
				Laser_player_fire_spread( objp, SPREADFIRE_ID, 6, F1_0/16, 0, 0, shot_orientation);
				Laser_player_fire_spread( objp, SPREADFIRE_ID, 6, -F1_0/16, 0, 0, shot_orientation);
				Laser_player_fire_spread( objp, SPREADFIRE_ID, 6, 0, 0, 1, shot_orientation);
			} else {
				Laser_player_fire_spread( objp, SPREADFIRE_ID, 6, 0, F1_0/16, 0, shot_orientation);
				Laser_player_fire_spread( objp, SPREADFIRE_ID, 6, 0, -F1_0/16, 0, shot_orientation);
				Laser_player_fire_spread( objp, SPREADFIRE_ID, 6, 0, 0, 1, shot_orientation);
			}
			break;

		case PLASMA_INDEX:
			Laser_player_fire( objp, PLASMA_ID, 0, 1, shot_orientation);
			Laser_player_fire( objp, PLASMA_ID, 1, 0, shot_orientation);
			if (nfires > 1) {
				Laser_player_fire_spread_delay( objp, PLASMA_ID, 0, 0, 0, FrameTime/2, 1, shot_orientation);
				Laser_player_fire_spread_delay( objp, PLASMA_ID, 1, 0, 0, FrameTime/2, 0, shot_orientation);
			}
			break;

		case FUSION_INDEX: {
			vms_vector	force_vec;

			Laser_player_fire( objp, FUSION_ID, 0, 1, shot_orientation);
			Laser_player_fire( objp, FUSION_ID, 1, 1, shot_orientation);

			flags = (sbyte)(Fusion_charge >> 12);

			Fusion_charge = 0;

			force_vec.x = -(objp->orient.fvec.x << 7);
			force_vec.y = -(objp->orient.fvec.y << 7);
			force_vec.z = -(objp->orient.fvec.z << 7);
			phys_apply_force(objp, force_vec);

			force_vec.x = (force_vec.x >> 4) + d_rand() - 16384;
			force_vec.y = (force_vec.y >> 4) + d_rand() - 16384;
			force_vec.z = (force_vec.z >> 4) + d_rand() - 16384;
			phys_apply_rot(objp, force_vec);

		}
			break;
#if defined(DXX_BUILD_DESCENT_II)
		case GAUSS_INDEX: {
			//	Only make sound for 1/4 of vulcan bullets.
			int	make_sound = 1;
			//if (d_rand() > 24576)
			//	make_sound = 1;

			Laser_player_fire_spread( objp, GAUSS_ID, 6, (d_rand()/8 - 32767/16)/5, (d_rand()/8 - 32767/16)/5, make_sound, shot_orientation);
			if (nfires > 1) {
				Laser_player_fire_spread( objp, GAUSS_ID, 6, (d_rand()/8 - 32767/16)/5, (d_rand()/8 - 32767/16)/5, 0, shot_orientation);
				if (nfires > 2) {
					Laser_player_fire_spread( objp, GAUSS_ID, 6, (d_rand()/8 - 32767/16)/5, (d_rand()/8 - 32767/16)/5, 0, shot_orientation);
				}
			}
			break;
		}
		case HELIX_INDEX: {
			int helix_orient;
			fix spreadr,spreadu;
			helix_orient = (flags >> LASER_HELIX_SHIFT) & LASER_HELIX_MASK;
			switch(helix_orient) {

				case 0: spreadr =  F1_0/16; spreadu = 0;       break; // Vertical
				case 1: spreadr =  F1_0/17; spreadu = F1_0/42; break; //  22.5 degrees
				case 2: spreadr =  F1_0/22; spreadu = F1_0/22; break; //  45   degrees
				case 3: spreadr =  F1_0/42; spreadu = F1_0/17; break; //  67.5 degrees
				case 4: spreadr =  0;       spreadu = F1_0/16; break; //  90   degrees
				case 5: spreadr = -F1_0/42; spreadu = F1_0/17; break; // 112.5 degrees
				case 6: spreadr = -F1_0/22; spreadu = F1_0/22; break; // 135   degrees
				case 7: spreadr = -F1_0/17; spreadu = F1_0/42; break; // 157.5 degrees
				default:
					Error("Invalid helix_orientation value %x\n",helix_orient);
			}

			Laser_player_fire_spread( objp, HELIX_ID, 6,  0,  0, 1, shot_orientation);
			Laser_player_fire_spread( objp, HELIX_ID, 6,  spreadr,  spreadu, 0, shot_orientation);
			Laser_player_fire_spread( objp, HELIX_ID, 6, -spreadr, -spreadu, 0, shot_orientation);
			Laser_player_fire_spread( objp, HELIX_ID, 6,  spreadr*2,  spreadu*2, 0, shot_orientation);
			Laser_player_fire_spread( objp, HELIX_ID, 6, -spreadr*2, -spreadu*2, 0, shot_orientation);
			break;
		}

		case PHOENIX_INDEX:
			Laser_player_fire( objp, PHOENIX_ID, 0, 1, shot_orientation);
			Laser_player_fire( objp, PHOENIX_ID, 1, 0, shot_orientation);
			if (nfires > 1) {
				Laser_player_fire_spread_delay( objp, PHOENIX_ID, 0, 0, 0, FrameTime/2, 1, shot_orientation);
				Laser_player_fire_spread_delay( objp, PHOENIX_ID, 1, 0, 0, FrameTime/2, 0, shot_orientation);
			}
			break;

		case OMEGA_INDEX:
			Laser_player_fire( objp, OMEGA_ID, 1, 1, shot_orientation);
			break;
#endif

#if defined(DXX_BUILD_DESCENT_II)
		case SUPER_LASER_INDEX:
#endif
		default:
			Int3();	//	Contact Yuan: Unknown Primary weapon type, setting to 0.
			Primary_weapon = 0;
	}

	// Set values to be recognized during comunication phase, if we are the
	//  one shooting
	if ((Game_mode & GM_MULTI) && objp == Players[Player_num].objnum)
		multi_send_fire(weapon_num, level, flags, nfires, object_none, object_none);

	return nfires;
}

#define	MAX_SMART_DISTANCE	(F1_0*150)
#define	MAX_OBJDISTS			30

//	-------------------------------------------------------------------------------------------
//	if goal_obj == -1, then create random vector
static objptridx_t create_homing_missile(const vobjptridx_t objp, const objptridx_t goal_obj, enum weapon_type_t objtype, int make_sound)
{
	vms_vector	vector_to_goal;
	//vms_vector	goal_pos;

	if (goal_obj == object_none) {
		make_random_vector(vector_to_goal);
	} else {
		vm_vec_normalized_dir_quick(vector_to_goal, goal_obj->pos, objp->pos);
		const auto random_vector = make_random_vector();
		vm_vec_scale_add2(vector_to_goal, random_vector, F1_0/4);
		vm_vec_normalize_quick(vector_to_goal);
	}

	//	Create a vector towards the goal, then add some noise to it.
	auto objnum = Laser_create_new(vector_to_goal, objp->pos, objp->segnum, objp, objtype, make_sound);
	if (objnum == object_none)
		return objnum;

	// Fixed to make sure the right person gets credit for the kill

//	Objects[objnum].ctype.laser_info.parent_num = objp->ctype.laser_info.parent_num;
//	Objects[objnum].ctype.laser_info.parent_type = objp->ctype.laser_info.parent_type;
//	Objects[objnum].ctype.laser_info.parent_signature = objp->ctype.laser_info.parent_signature;
	objnum->ctype.laser_info.track_goal = goal_obj;
	return objnum;
}

//-----------------------------------------------------------------------------
// Create the children of a smart bomb, which is a bunch of homing missiles.
void create_smart_children(const vobjptridx_t objp, int num_smart_children)
{
	int parent_type, parent_num;
	int numobjs=0;
	objnum_t objlist[MAX_OBJDISTS];
	enum weapon_type_t blob_id;

#if defined(DXX_BUILD_DESCENT_I)
	parent_type = objp->ctype.laser_info.parent_type;
	parent_num = objp->ctype.laser_info.parent_num;
	if (get_weapon_id(objp) == SMART_ID)
#elif defined(DXX_BUILD_DESCENT_II)
	if (objp->type == OBJ_WEAPON) {
		parent_type = objp->ctype.laser_info.parent_type;
		parent_num = objp->ctype.laser_info.parent_num;
	} else if (objp->type == OBJ_ROBOT) {
		parent_type = OBJ_ROBOT;
		parent_num = objp;
	} else {
		Int3();	//	Hey, what kind of object is this!?
		parent_type = 0;
		parent_num = 0;
	}

#ifndef NDEBUG
	if ((objp->type == OBJ_WEAPON) && ((objp->id == SMART_ID) || (objp->id == SUPERPROX_ID) || (objp->id == ROBOT_SUPERPROX_ID) || (objp->id == EARTHSHAKER_ID)))
		Assert(Weapon_info[objp->id].children != -1);
#endif

	if (objp->type == OBJ_WEAPON && objp->id == EARTHSHAKER_ID)
		blast_nearby_glass(objp, Weapon_info[EARTHSHAKER_ID].strength[Difficulty_level]);

	if (((objp->type == OBJ_WEAPON) && (Weapon_info[objp->id].children != -1)) || (objp->type == OBJ_ROBOT))
#endif
	{
		if (Game_mode & GM_MULTI)
			d_srand(8321L);

		range_for (const auto objnum, highest_valid(Objects))
		{
			object *curobjp = &Objects[objnum];

			if ((((curobjp->type == OBJ_ROBOT) && (!curobjp->ctype.ai_info.CLOAKED)) || (curobjp->type == OBJ_PLAYER)) && (objnum != parent_num)) {
				fix dist;

				if (curobjp->type == OBJ_PLAYER)
				{
					if ((parent_type == OBJ_PLAYER) && (Game_mode & GM_MULTI_COOP))
						continue;
					if ((Game_mode & GM_TEAM) && (get_team(get_player_id(curobjp)) == get_team(get_player_id(&Objects[parent_num]))))
						continue;
					if (Players[get_player_id(curobjp)].flags & PLAYER_FLAGS_CLOAKED)
						continue;
				}

				//	Robot blobs can't track robots.
				if (curobjp->type == OBJ_ROBOT) {
					if (parent_type == OBJ_ROBOT)
						continue;

#if defined(DXX_BUILD_DESCENT_II)
					//	Your shots won't track the buddy.
					if (parent_type == OBJ_PLAYER)
						if (Robot_info[curobjp->id].companion)
							continue;
#endif
				}

				dist = vm_vec_dist_quick(objp->pos, curobjp->pos);
				if (dist < MAX_SMART_DISTANCE) {
					int oovis;

					oovis = object_to_object_visibility(objp, curobjp, FQ_TRANSWALL);

					if (oovis) {
						objlist[numobjs] = objnum;
						numobjs++;
						if (numobjs >= MAX_OBJDISTS) {
							numobjs = MAX_OBJDISTS;
							break;
						}
					}
				}
			}
		}

		//	Get type of weapon for child from parent.
#if defined(DXX_BUILD_DESCENT_I)
		if (parent_type == OBJ_PLAYER) {
			blob_id = PLAYER_SMART_HOMING_ID;
		} else {
			blob_id = ((N_weapon_types<ROBOT_SMART_HOMING_ID)?(PLAYER_SMART_HOMING_ID):(ROBOT_SMART_HOMING_ID)); // NOTE: Shareware & reg 1.0 do not have their own Smart structure for bots. It was introduced in 1.4 to make Smart blobs from lvl 7 boss easier to dodge. So if we do not have this type, revert to player's Smart behaviour..,
		}
#elif defined(DXX_BUILD_DESCENT_II)
		if (objp->type == OBJ_WEAPON) {
			blob_id = (enum weapon_type_t) Weapon_info[objp->id].children;
			Assert(blob_id != -1);		//	Hmm, missing data in bitmaps.tbl.  Need "children=NN" parameter.
		} else {
			Assert(objp->type == OBJ_ROBOT);
			blob_id = ROBOT_SMART_HOMING_ID;
		}
#endif

		objnum_t last_sel_objnum = object_none;
		for (int i=0; i<num_smart_children; i++) {
			objptridx_t sel_objnum = object_none;
			if (numobjs)
				sel_objnum = objlist[(d_rand() * numobjs) >> 15];
			if (numobjs > 1)
				while (sel_objnum == last_sel_objnum)
					sel_objnum = objlist[(d_rand() * numobjs) >> 15];
			create_homing_missile(objp, sel_objnum, blob_id, (i==0)?1:0);
			last_sel_objnum = sel_objnum;
		}
	}
}

int Missile_gun = 0;
int Proximity_dropped = 0;

#if defined(DXX_BUILD_DESCENT_II)
int Smartmines_dropped=0;

//give up control of the guided missile
void release_guided_missile(int player_num)
{
	if (player_num == Player_num)
	 {
	  if (Guided_missile[player_num]==NULL)
			return;

		Missile_viewer = Guided_missile[player_num];
		if (Game_mode & GM_MULTI)
		 multi_send_guided_info (Guided_missile[Player_num],1);
		if (Newdemo_state==ND_STATE_RECORDING)
		 newdemo_record_guided_end();
	 }

	Guided_missile[player_num] = NULL;
}
#endif

//	-------------------------------------------------------------------------------------------
//changed on 31/3/10 by kreatordxx to distinguish between drop bomb and secondary fire
void do_missile_firing(int drop_bomb)
{
	int gun_flag=0;
	int bomb = which_bomb();
	int weapon = (drop_bomb) ? bomb : Secondary_weapon;
	fix fire_frame_overhead = 0;

	Network_laser_track = object_none;

	Assert(weapon < MAX_SECONDARY_WEAPONS);

	if (GameTime64 - Next_missile_fire_time <= FrameTime) // if firing is prolonged by FrameTime overhead, let's try to fix that.
		fire_frame_overhead = GameTime64 - Next_missile_fire_time;

#if defined(DXX_BUILD_DESCENT_II)
	if (Guided_missile[Player_num] && Guided_missile[Player_num]->signature==Guided_missile_sig[Player_num]) {
		release_guided_missile(Player_num);
		Next_missile_fire_time = GameTime64 + Weapon_info[Secondary_weapon_to_weapon_info[weapon]].fire_wait - fire_frame_overhead;
		return;
	}
#endif

	if (!Player_is_dead && (Players[Player_num].secondary_ammo[weapon] > 0))	{

		enum weapon_type_t weapon_index;
		int weapon_gun;

		Players[Player_num].secondary_ammo[weapon]--;

		weapon_index = (enum weapon_type_t) Secondary_weapon_to_weapon_info[weapon];

		if (!cheats.rapidfire)
			Next_missile_fire_time = GameTime64 + Weapon_info[weapon_index].fire_wait - fire_frame_overhead;
		else
			Next_missile_fire_time = GameTime64 + (F1_0/25) - fire_frame_overhead;

		weapon_gun = Secondary_weapon_to_gun_num[weapon];

		if (weapon_gun==4) {		//alternate left/right
			weapon_gun += (gun_flag = (Missile_gun & 1));
			Missile_gun++;
		}

		auto objnum = Laser_player_fire(vobjptridx(ConsoleObject), weapon_index, weapon_gun, 1, Objects[Players[Player_num].objnum].orient.fvec);

		if (weapon == PROXIMITY_INDEX) {
			if (++Proximity_dropped == 4) {
				Proximity_dropped = 0;
				maybe_drop_net_powerup(POW_PROXIMITY_WEAPON);
			}
		}
#if defined(DXX_BUILD_DESCENT_II)
		else if (weapon == SMART_MINE_INDEX) {
			if (++Smartmines_dropped == 4) {
				Smartmines_dropped = 0;
				maybe_drop_net_powerup(POW_SMART_MINE);
			}
		}
#endif
		else if (weapon != CONCUSSION_INDEX)
			maybe_drop_net_powerup(Secondary_weapon_to_powerup[weapon]);

#if defined(DXX_BUILD_DESCENT_I)
		if (weapon == MEGA_INDEX)
#elif defined(DXX_BUILD_DESCENT_II)
		if (weapon == MEGA_INDEX || weapon == SMISSILE5_INDEX)
#endif
		{
			vms_vector force_vec;

			force_vec.x = -(ConsoleObject->orient.fvec.x << 7);
			force_vec.y = -(ConsoleObject->orient.fvec.y << 7);
			force_vec.z = -(ConsoleObject->orient.fvec.z << 7);
			phys_apply_force(ConsoleObject, force_vec);

			force_vec.x = (force_vec.x >> 4) + d_rand() - 16384;
			force_vec.y = (force_vec.y >> 4) + d_rand() - 16384;
			force_vec.z = (force_vec.z >> 4) + d_rand() - 16384;
			phys_apply_rot(ConsoleObject, force_vec);
		}

		if (Game_mode & GM_MULTI)
		{
			if (weapon_index_is_player_bomb(weapon))
				multi_send_fire(weapon+MISSILE_ADJUST, 0, gun_flag, 1, Network_laser_track, objnum);
			else
				multi_send_fire(weapon+MISSILE_ADJUST, 0, gun_flag, 1, Network_laser_track, object_none);
		}

		// don't autoselect if dropping prox and prox not current weapon
		if (!drop_bomb || Secondary_weapon == bomb)
			auto_select_weapon(1);		//select next missile, if this one out of ammo
	}
}
