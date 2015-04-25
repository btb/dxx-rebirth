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
 * Bitmap and Palette loading functions.
 *
 */

#pragma once

#include <physfs.h>

struct bitmap_index;

#ifdef __cplusplus

struct grs_bitmap;

#if defined(DXX_BUILD_DESCENT_I)
#define MAX_TEXTURES		800
#elif defined(DXX_BUILD_DESCENT_II)
#define MAX_TEXTURES    1200
#endif

//tmapinfo flags
#define TMI_VOLATILE    1   //this material blows up when hit
#if defined(DXX_BUILD_DESCENT_II)
#define TMI_WATER       2   //this material is water
#define TMI_FORCE_FIELD 4   //this is force field - flares don't stick
#define TMI_GOAL_BLUE   8   //this is used to remap the blue goal
#define TMI_GOAL_RED    16  //this is used to remap the red goal
#define TMI_GOAL_HOARD  32  //this is used to remap the goals
#endif

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
#include "inferno.h"
struct tmap_info : prohibit_void_ptr<tmap_info>
{
#if defined(DXX_BUILD_DESCENT_I)
	d_fname filename;
	ubyte			flags;
	fix			lighting;		// 0 to 1
	fix			damage;			//how much damage being against this does
	int			eclip_num;		//if not -1, the eclip that changes this   
#define N_COCKPIT_BITMAPS 4
#elif defined(DXX_BUILD_DESCENT_II)
	fix     lighting;  //how much light this casts
	fix     damage;    //how much damage being against this does (for lava)
	short   eclip_num; //the eclip that changes this, or -1
	short   destroyed; //bitmap to show when destroyed, or -1
	short   slide_u,slide_v;    //slide rates of texture, stored in 8:8 fix
	ubyte   flags;     //values defined above
	#ifdef EDITOR
	d_fname filename;       //used by editor to remap textures
	#endif

#define TMAP_INFO_SIZE 20   // how much space it takes up on disk
#define N_COCKPIT_BITMAPS 6
#endif
};

extern int Num_object_types;

struct player_ship;
//right now there's only one player ship, but we can have another by
//adding an array and setting the pointer to the active ship.
extern struct player_ship only_player_ship;
static struct player_ship *const Player_ship=&only_player_ship;
extern unsigned Num_cockpits;
extern array<bitmap_index, N_COCKPIT_BITMAPS> cockpit_bitmap;
#ifdef EDITOR
extern array<short, MAX_TEXTURES> tmap_xlate_table;
#endif

extern unsigned Num_tmaps;
extern array<tmap_info, MAX_TEXTURES> TmapInfo;

// Initializes the palette, bitmap system...
void gamedata_close();
int gamedata_init();
void bm_close();

// Initializes the Texture[] array of bmd_bitmap structures.
void init_textures();

#if defined(DXX_BUILD_DESCENT_I)

#define OL_ROBOT 				1
#define OL_HOSTAGE 			2
#define OL_POWERUP 			3
#define OL_CONTROL_CENTER	4
#define OL_PLAYER				5
#define OL_CLUTTER			6		//some sort of misc object
#define OL_EXIT				7		//the exit model for external scenes

#define	MAX_OBJTYPE			100

extern int Num_total_object_types;		//	Total number of object types, including robots, hostages, powerups, control centers, faces
extern sbyte	ObjType[MAX_OBJTYPE];		// Type of an object, such as Robot, eg if ObjType[11] == OL_ROBOT, then object #11 is a robot
extern sbyte	ObjId[MAX_OBJTYPE];			// ID of a robot, within its class, eg if ObjType[11] == 3, then object #11 is the third robot
extern fix	ObjStrength[MAX_OBJTYPE];	// initial strength of each object

#define MAX_OBJ_BITMAPS				210

#elif defined(DXX_BUILD_DESCENT_II)

//the model number of the marker object
extern int Marker_model_num;
extern int Robot_replacements_loaded;
#define MAX_OBJ_BITMAPS     610
extern int N_ObjBitmaps;
extern int extra_bitmap_num;
#endif

extern int  Num_object_subtypes;     // Number of possible IDs for the current type of object to be placed

extern array<bitmap_index, MAX_OBJ_BITMAPS> ObjBitmaps;
extern array<ushort, MAX_OBJ_BITMAPS> ObjBitmapPtrs;
extern int First_multi_bitmap_num;
void compute_average_rgb(grs_bitmap *bm, array<fix, 3> &rgb);
#endif

// Initializes all bitmaps from BITMAPS.TBL file.
int gamedata_read_tbl(int pc_shareware);

extern void bm_read_all(PHYSFS_file * fp);

int load_exit_models();
void load_robot_replacements(const d_fname &level_name);
void bm_read_extra_robots(const char *fname,int type);
#if defined(DXX_BUILD_DESCENT_I)
void properties_read_cmp(PHYSFS_file * fp);
#endif
int ds_load(int skip, const char * filename );
int compute_average_pixel(grs_bitmap *n);

#if defined(DXX_BUILD_DESCENT_II)
//these values are the number of each item in the release of d2
//extra items added after the release get written in an additional hamfile
static const unsigned N_D2_ROBOT_TYPES = 66;
static const unsigned N_D2_ROBOT_JOINTS = 1145;
static const unsigned N_D2_OBJBITMAPS = 422;
static const unsigned N_D2_OBJBITMAPPTRS = 502;
static const unsigned N_D2_WEAPON_TYPES = 62;
#endif

#endif
