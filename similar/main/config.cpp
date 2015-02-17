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


	auto infile = PHYSFSX_openReadBuffered("descent.cfg");
	if (!infile)
	{
		return 1;
	}

	// to be fully safe, assume the whole cfg consists of one big line
	for (PHYSFSX_gets_line_t<0> line(PHYSFS_fileLength(infile) + 1); !PHYSFS_eof(infile);)
	{
		PHYSFSX_fgets(line, infile);
		const auto lb = line.begin();
		const auto eol = std::find(lb, line.end(), 0);
		if (eol == line.end())
			continue;
		auto eq = std::find(lb, eol, '=');
		if (eq == eol)
			continue;
		auto value = std::next(eq);
		if (!d_stricmp(lb, GameCfg.DigiVolume.name))
			GameCfg.DigiVolume = value;
		else if (!d_stricmp(lb, GameCfg.MusicVolume.name))
			GameCfg.MusicVolume = value;
		else if (!d_stricmp(lb, GameCfg.ReverseStereo.name))
			GameCfg.ReverseStereo = value;
		else if (!d_stricmp(lb, GameCfg.OrigTrackOrder.name))
			GameCfg.OrigTrackOrder = value;
		else if (!d_stricmp(lb, GameCfg.MusicType.name))
			GameCfg.MusicType = value;
		else if (!d_stricmp(lb, GameCfg.CMLevelMusicPlayOrder.name))
			GameCfg.CMLevelMusicPlayOrder = value;
		else if (!d_stricmp(lb, GameCfg.CMLevelMusicTrack[0].name))
			GameCfg.CMLevelMusicTrack[0] = value;
		else if (!d_stricmp(lb, GameCfg.CMLevelMusicTrack[1].name))
			GameCfg.CMLevelMusicTrack[1] = value;
		else if (!d_stricmp(lb, GameCfg.CMLevelMusicPath.name))
			GameCfg.CMLevelMusicPath = value;
		else if (!d_stricmp(lb, GameCfg.CMMiscMusic[SONG_TITLE].name))
			GameCfg.CMMiscMusic[SONG_TITLE] = value;
		else if (!d_stricmp(lb, GameCfg.CMMiscMusic[SONG_BRIEFING].name))
			GameCfg.CMMiscMusic[SONG_BRIEFING] = value;
		else if (!d_stricmp(lb, GameCfg.CMMiscMusic[SONG_ENDLEVEL].name))
			GameCfg.CMMiscMusic[SONG_ENDLEVEL] = value;
		else if (!d_stricmp(lb, GameCfg.CMMiscMusic[SONG_ENDGAME].name))
			GameCfg.CMMiscMusic[SONG_ENDGAME] = value;
		else if (!d_stricmp(lb, GameCfg.CMMiscMusic[SONG_CREDITS].name))
			GameCfg.CMMiscMusic[SONG_CREDITS] = value;
		else if (!d_stricmp(lb, GameCfg.GammaLevel.name))
		{
			GameCfg.GammaLevel = value;
			gr_palette_set_gamma( GameCfg.GammaLevel );
		}
		else if (!d_stricmp(lb, GameCfg.LastPlayer.name))
		{
			char temp[CALLSIGN_LEN + 1];
			strncpy(temp, value, CALLSIGN_LEN + 1);
			d_strlwr(temp);
			GameCfg.LastPlayer = temp;
		}
		else if (!d_stricmp(lb, GameCfg.LastMission.name))
			GameCfg.LastMission = value;
		else if (!d_stricmp(lb, GameCfg.ResolutionX.name))
			GameCfg.ResolutionX = value;
		else if (!d_stricmp(lb, GameCfg.ResolutionY.name))
			GameCfg.ResolutionY = value;
		else if (!d_stricmp(lb, GameCfg.AspectX.name))
			GameCfg.AspectX = value;
		else if (!d_stricmp(lb, GameCfg.AspectY.name))
			GameCfg.AspectY = value;
		else if (!d_stricmp(lb, GameCfg.WindowMode.name))
			GameCfg.WindowMode = value;
		else if (!d_stricmp(lb, GameCfg.TexFilt.name))
			GameCfg.TexFilt = value;
#if defined(DXX_BUILD_DESCENT_II)
		else if (!d_stricmp(lb, GameCfg.MovieTexFilt.name))
			GameCfg.MovieTexFilt = value;
		else if (!d_stricmp(lb, GameCfg.MovieSubtitles.name))
			GameCfg.MovieSubtitles = value;
#endif
		else if (!d_stricmp(lb, GameCfg.VSync.name))
			GameCfg.VSync = value;
		else if (!d_stricmp(lb, GameCfg.Multisample.name))
			GameCfg.Multisample = value;
		else if (!d_stricmp(lb, GameCfg.FPSIndicator.name))
			GameCfg.FPSIndicator = value;
		else if (!d_stricmp(lb, GameCfg.Grabinput.name))
			GameCfg.Grabinput = value;
	}
	if ( GameCfg.DigiVolume > 8 ) GameCfg.DigiVolume = 8;
	if ( GameCfg.MusicVolume > 8 ) GameCfg.MusicVolume = 8;

	if (GameCfg.ResolutionX >= 320 && GameCfg.ResolutionY >= 200)
		Game_screen_mode = SM(GameCfg.ResolutionX,GameCfg.ResolutionY);

	return 0;
}

int WriteConfigFile()
{
	GameCfg.GammaLevel = gr_palette_get_gamma();

	auto infile = PHYSFSX_openWriteBuffered("descent.cfg");
	if (!infile)
	{
		return 1;
	}
	PHYSFSX_printf(infile, "%s=%d\n", GameCfg.DigiVolume.name, static_cast<int>(GameCfg.DigiVolume));
	PHYSFSX_printf(infile, "%s=%d\n", GameCfg.MusicVolume.name, static_cast<int>(GameCfg.MusicVolume));
	PHYSFSX_printf(infile, "%s=%d\n", GameCfg.ReverseStereo.name, static_cast<int>(GameCfg.ReverseStereo));
	PHYSFSX_printf(infile, "%s=%d\n", GameCfg.OrigTrackOrder.name, static_cast<int>(GameCfg.OrigTrackOrder));
	PHYSFSX_printf(infile, "%s=%d\n", GameCfg.MusicType.name, static_cast<int>(GameCfg.MusicType));
	PHYSFSX_printf(infile, "%s=%d\n", GameCfg.CMLevelMusicPlayOrder.name, static_cast<int>(GameCfg.CMLevelMusicPlayOrder));
	PHYSFSX_printf(infile, "%s=%d\n", GameCfg.CMLevelMusicTrack[0].name, static_cast<int>(GameCfg.CMLevelMusicTrack[0]));
	PHYSFSX_printf(infile, "%s=%d\n", GameCfg.CMLevelMusicTrack[1].name, static_cast<int>(GameCfg.CMLevelMusicTrack[1]));
	PHYSFSX_printf(infile, "%s=%s\n", GameCfg.CMLevelMusicPath.name, static_cast<const char *>(GameCfg.CMLevelMusicPath));
	PHYSFSX_printf(infile, "%s=%s\n", GameCfg.CMMiscMusic[SONG_TITLE].name, static_cast<const char *>(GameCfg.CMMiscMusic[SONG_TITLE]));
	PHYSFSX_printf(infile, "%s=%s\n", GameCfg.CMMiscMusic[SONG_BRIEFING].name, static_cast<const char *>(GameCfg.CMMiscMusic[SONG_BRIEFING]));
	PHYSFSX_printf(infile, "%s=%s\n", GameCfg.CMMiscMusic[SONG_ENDLEVEL].name, static_cast<const char *>(GameCfg.CMMiscMusic[SONG_ENDLEVEL]));
	PHYSFSX_printf(infile, "%s=%s\n", GameCfg.CMMiscMusic[SONG_ENDGAME].name, static_cast<const char *>(GameCfg.CMMiscMusic[SONG_ENDGAME]));
	PHYSFSX_printf(infile, "%s=%s\n", GameCfg.CMMiscMusic[SONG_CREDITS].name, static_cast<const char *>(GameCfg.CMMiscMusic[SONG_CREDITS]));
	PHYSFSX_printf(infile, "%s=%d\n", GameCfg.GammaLevel.name, static_cast<int>(GameCfg.GammaLevel));
	PHYSFSX_printf(infile, "%s=%s\n", GameCfg.LastPlayer.name, static_cast<const char *>(Players[Player_num].callsign));
	PHYSFSX_printf(infile, "%s=%s\n", GameCfg.LastMission.name, static_cast<const char *>(GameCfg.LastMission));
	PHYSFSX_printf(infile, "%s=%i\n", GameCfg.ResolutionX.name, SM_W(Game_screen_mode));
	PHYSFSX_printf(infile, "%s=%i\n", GameCfg.ResolutionY.name, SM_H(Game_screen_mode));
	PHYSFSX_printf(infile, "%s=%i\n", GameCfg.AspectX.name, static_cast<int>(GameCfg.AspectX));
	PHYSFSX_printf(infile, "%s=%i\n", GameCfg.AspectY.name, static_cast<int>(GameCfg.AspectY));
	PHYSFSX_printf(infile, "%s=%i\n", GameCfg.WindowMode.name, static_cast<int>(GameCfg.WindowMode));
	PHYSFSX_printf(infile, "%s=%i\n", GameCfg.TexFilt.name, static_cast<int>(GameCfg.TexFilt));
#if defined(DXX_BUILD_DESCENT_II)
	PHYSFSX_printf(infile, "%s=%i\n", GameCfg.MovieTexFilt.name, static_cast<int>(GameCfg.MovieTexFilt));
	PHYSFSX_printf(infile, "%s=%i\n", GameCfg.MovieSubtitles.name, static_cast<int>(GameCfg.MovieSubtitles));
#endif
	PHYSFSX_printf(infile, "%s=%i\n", GameCfg.VSync.name, static_cast<int>(GameCfg.VSync));
	PHYSFSX_printf(infile, "%s=%i\n", GameCfg.Multisample.name, static_cast<int>(GameCfg.Multisample));
	PHYSFSX_printf(infile, "%s=%i\n", GameCfg.FPSIndicator.name, static_cast<int>(GameCfg.FPSIndicator));
	PHYSFSX_printf(infile, "%s=%i\n", GameCfg.Grabinput.name, static_cast<int>(GameCfg.Grabinput));
	return 0;
}
