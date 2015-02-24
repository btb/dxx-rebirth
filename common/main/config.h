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
 * prototype definitions for descent.cfg reading/writing
 *
 */


#ifndef _CONFIG_H
#define _CONFIG_H

#include "player.h"
#include "mission.h"

#ifdef __cplusplus
#include "pack.h"
#include "compiler-array.h"
#include "ntstring.h"

struct Cfg : prohibit_void_ptr<Cfg>
{
	cvar_t DigiVolume             = { "DigiVolume",            "0", CVAR_ARCHIVE };
	cvar_t MusicVolume            = { "MusicVolume",           "0", CVAR_ARCHIVE };
	cvar_t ReverseStereo          = { "ReverseStereo",         "0", CVAR_ARCHIVE };
	cvar_t OrigTrackOrder         = { "OrigTrackOrder",        "0", CVAR_ARCHIVE };
	cvar_t MusicType              = { "MusicType",             "0", CVAR_ARCHIVE };
	cvar_t CMLevelMusicPlayOrder  = { "CMLevelMusicPlayOrder", "0", CVAR_ARCHIVE };
	cvar_t CMLevelMusicTrack[2] = { { "CMLevelMusicTrack0",    "0", CVAR_ARCHIVE },
	                                { "CMLevelMusicTrack1",    "0", CVAR_ARCHIVE } };
	cvar_t CMLevelMusicPath       = { "CMLevelMusicPath",      "0", CVAR_ARCHIVE };
	cvar_t CMMiscMusic[5]       = { { "CMMiscMusic0",          "0", CVAR_ARCHIVE },
	                                { "CMMiscMusic1",          "0", CVAR_ARCHIVE },
	                                { "CMMiscMusic2",          "0", CVAR_ARCHIVE },
	                                { "CMMiscMusic3",          "0", CVAR_ARCHIVE },
	                                { "CMMiscMusic4",          "0", CVAR_ARCHIVE } };
	cvar_t GammaLevel             = { "GammaLevel",            "0", CVAR_ARCHIVE };
	cvar_t LastPlayer             = { "LastPlayer",            "0", CVAR_ARCHIVE };
	cvar_t LastMission            = { "LastMission",           "0", CVAR_ARCHIVE };
	cvar_t ResolutionX            = { "ResolutionX",           "0", CVAR_ARCHIVE };
	cvar_t ResolutionY            = { "ResolutionY",           "0", CVAR_ARCHIVE };
	cvar_t AspectX                = { "AspectX",               "0", CVAR_ARCHIVE };
	cvar_t AspectY                = { "AspectY",               "0", CVAR_ARCHIVE };
	cvar_t WindowMode             = { "WindowMode",            "0", CVAR_ARCHIVE };
	cvar_t TexFilt                = { "TexFilt",               "0", CVAR_ARCHIVE };
#if defined(DXX_BUILD_DESCENT_II)
	cvar_t MovieTexFilt           = { "MovieTexFilt",          "0", CVAR_ARCHIVE };
	cvar_t MovieSubtitles         = { "MovieSubtitles",        "0", CVAR_ARCHIVE };
#endif
	cvar_t VSync                  = { "VSync",                 "0", CVAR_ARCHIVE };
	cvar_t Multisample            = { "Multisample",           "0", CVAR_ARCHIVE };
	cvar_t FPSIndicator           = { "FPSIndicator",          "0", CVAR_ARCHIVE };
	cvar_t Grabinput              = { "GrabInput",             "0", CVAR_ARCHIVE };
};

extern struct Cfg GameCfg;

//#ifdef USE_SDLMIXER
//#define EXT_MUSIC_ON (GameCfg.SndEnableRedbook || GameCfg.JukeboxOn)
//#else
//#define EXT_MUSIC_ON (GameCfg.SndEnableRedbook)		// JukeboxOn shouldn't do anything if it's not supported
//#endif

extern int ReadConfigFile(void);
extern int WriteConfigFile(void);

#endif

#endif
