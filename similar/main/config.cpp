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
 * contains routine(s) to read in the configuration file which contains
 * game configuration stuff like detail level, sound card, etc
 *
 */

#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "config.h"
#include "pstypes.h"
#include "game.h"
#include "songs.h"
#include "kconfig.h"
#include "palette.h"
#include "args.h"
#include "player.h"
#include "mission.h"
#include "u_mem.h"
#include "physfsx.h"
#include "nvparse.h"
#include "strutil.h"

#include "compiler-make_unique.h"

struct Cfg GameCfg;


static int config_initialized;

static void config_init(void)
{
	int i;

	/* make sure our cvars are registered */
	cvar_registervariable(&GameCfg.DigiVolume);
	cvar_registervariable(&GameCfg.MusicVolume);
	cvar_registervariable(&GameCfg.ReverseStereo);
	cvar_registervariable(&GameCfg.OrigTrackOrder);
	cvar_registervariable(&GameCfg.MusicType);
	cvar_registervariable(&GameCfg.CMLevelMusicPlayOrder);
	for (i = 0; i < 2; i++)
		cvar_registervariable(&GameCfg.CMLevelMusicTrack[i]);
	cvar_registervariable(&GameCfg.CMLevelMusicPath);
	for (i = 0; i < 5; i++)
		cvar_registervariable(&GameCfg.CMMiscMusic[i]);
	cvar_registervariable(&GameCfg.GammaLevel);
	cvar_registervariable(&GameCfg.LastPlayer);
	cvar_registervariable(&GameCfg.LastMission);
	cvar_registervariable(&GameCfg.ResolutionX);
	cvar_registervariable(&GameCfg.ResolutionY);
	cvar_registervariable(&GameCfg.AspectX);
	cvar_registervariable(&GameCfg.AspectY);
	cvar_registervariable(&GameCfg.WindowMode);
	cvar_registervariable(&GameCfg.TexFilt);
#if defined(DXX_BUILD_DESCENT_II)
	cvar_registervariable(&GameCfg.MovieTexFilt);
	cvar_registervariable(&GameCfg.MovieSubtitles);
#endif
	cvar_registervariable(&GameCfg.VSync);
	cvar_registervariable(&GameCfg.Multisample);
	cvar_registervariable(&GameCfg.FPSIndicator);
	cvar_registervariable(&GameCfg.Grabinput);

	config_initialized = 1;
}


int ReadConfigFile()
{
	if (!config_initialized)
		config_init();

	// set defaults
	GameCfg.DigiVolume = 8;
	GameCfg.MusicVolume = 8;
	GameCfg.ReverseStereo = 0;
	GameCfg.OrigTrackOrder = 0;
#if defined(__APPLE__) && defined(__MACH__)
	GameCfg.MusicType = MUSIC_TYPE_REDBOOK;
#else
	GameCfg.MusicType = MUSIC_TYPE_BUILTIN;
#endif
	GameCfg.CMLevelMusicPlayOrder = MUSIC_CM_PLAYORDER_CONT;
	GameCfg.CMLevelMusicTrack[0] = -1;
	GameCfg.CMLevelMusicTrack[1] = -1;
	GameCfg.CMLevelMusicPath = "";
	GameCfg.CMMiscMusic[0] = "";
	GameCfg.CMMiscMusic[1] = "";
	GameCfg.CMMiscMusic[2] = "";
	GameCfg.CMMiscMusic[3] = "";
	GameCfg.CMMiscMusic[4] = "";
#if defined(__APPLE__) && defined(__MACH__)
	GameCfg.OrigTrackOrder = 1;
#if defined(DXX_BUILD_DESCENT_I)
	GameCfg.CMLevelMusicPlayOrder = MUSIC_CM_PLAYORDER_LEVEL;
	GameCfg.CMLevelMusicPath = "descent.m3u";
	cvar_set_cvarf(&GameCfg.CMMiscMusic[SONG_TITLE], "%s%s", PHYSFS_getUserDir(), "Music/iTunes/iTunes Music/Insanity/Descent/02 Primitive Rage.mp3");
	cvar_set_cvarf(&GameCfg.CMMiscMusic[SONG_CREDITS], "%s%s", PHYSFS_getUserDir(), "Music/iTunes/iTunes Music/Insanity/Descent/05 The Darkness Of Space.mp3");
#elif defined(DXX_BUILD_DESCENT_II)
	GameCfg.CMLevelMusicPath = "descent2.m3u";
	cvar_set_cvarf(&GameCfg.CMMiscMusic[SONG_TITLE], "%s%s", PHYSFS_getUserDir(), "Music/iTunes/iTunes Music/Redbook Soundtrack/Descent II, Macintosh CD-ROM/02 Title.mp3");
	cvar_set_cvarf(&GameCfg.CMMiscMusic[SONG_CREDITS], "%s%s", PHYSFS_getUserDir(), "Music/iTunes/iTunes Music/Redbook Soundtrack/Descent II, Macintosh CD-ROM/03 Crawl.mp3");
#endif
	cvar_set_cvarf(&GameCfg.CMMiscMusic[SONG_BRIEFING], "%s%s", PHYSFS_getUserDir(), "Music/iTunes/iTunes Music/Insanity/Descent/03 Outerlimits.mp3");
	cvar_set_cvarf(&GameCfg.CMMiscMusic[SONG_ENDLEVEL], "%s%s", PHYSFS_getUserDir(), "Music/iTunes/iTunes Music/Insanity/Descent/04 Close Call.mp3");
	cvar_set_cvarf(&GameCfg.CMMiscMusic[SONG_ENDGAME], "%s%s", PHYSFS_getUserDir(), "Music/iTunes/iTunes Music/Insanity/Descent/14 Insanity.mp3");
#endif
	GameCfg.GammaLevel = 0;
	GameCfg.LastPlayer = "";
	GameCfg.LastMission = "";
	GameCfg.ResolutionX = 640;
	GameCfg.ResolutionY = 480;
	GameCfg.AspectX = 3;
	GameCfg.AspectY = 4;
	GameCfg.WindowMode = 0;
	GameCfg.TexFilt = 0;
#if defined(DXX_BUILD_DESCENT_II)
	GameCfg.MovieTexFilt = 0;
	GameCfg.MovieSubtitles = 0;
#endif
	GameCfg.VSync = 0;
	GameCfg.Multisample = 0;
	GameCfg.FPSIndicator = 0;
	GameCfg.Grabinput = 1;


	if (!PHYSFSX_exists("descent.cfg", 1))
	{
		return 1;
	}

	cmd_append("exec descent.cfg");
	cmd_queue_flush();


	gr_palette_set_gamma( GameCfg.GammaLevel );

	if ( GameCfg.DigiVolume > 8 ) GameCfg.DigiVolume = 8;
	if ( GameCfg.MusicVolume > 8 ) GameCfg.MusicVolume = 8;

	if (GameCfg.ResolutionX >= 320 && GameCfg.ResolutionY >= 200)
		Game_screen_mode = SM(GameCfg.ResolutionX,GameCfg.ResolutionY);

	return 0;
}

int WriteConfigFile()
{
	GameCfg.GammaLevel = gr_palette_get_gamma();
	GameCfg.LastPlayer = Players[Player_num].callsign;
	GameCfg.ResolutionX = SM_W(Game_screen_mode);
	GameCfg.ResolutionY = SM_H(Game_screen_mode);

	auto infile = PHYSFSX_openWriteBuffered("descent.cfg");
	if (!infile)
	{
		return 1;
	}

	cvar_write(infile);

	return 0;
}
