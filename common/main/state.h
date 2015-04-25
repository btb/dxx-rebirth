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
 * Prototypes for state saving functions.
 *
 */

#pragma once

#if defined(DXX_BUILD_DESCENT_I)
#elif defined(DXX_BUILD_DESCENT_II)
#define SECRETB_FILENAME	PLAYER_DIRECTORY_STRING("secret.sgb")
#define SECRETC_FILENAME	PLAYER_DIRECTORY_STRING("secret.sgc")
#endif

#ifdef __cplusplus
#include <cstddef>

extern unsigned state_game_id;
extern int state_quick_item;

enum class secret_save
{
	none,
#if defined(DXX_BUILD_DESCENT_II)
	b,
	c,
#endif
};

enum class secret_restore
{
	none,
#if defined(DXX_BUILD_DESCENT_II)
	survived,
	died,
#endif
};

enum class blind_save
{
	no,
	yes,
};

int state_save_all_sub(const char *filename, const char *desc);

int state_get_save_file(char *fname, char * dsc, blind_save);
int state_get_restore_file(char *fname, blind_save);
int state_get_game_id(const char *filename);

#if defined(DXX_BUILD_DESCENT_I)
int state_restore_all_sub(const char *filename);
static inline int state_restore_all_sub(const char *filename, secret_restore)
{
	return state_restore_all_sub(filename);
}
static inline void set_pos_from_return_segment(void)
{
}
int state_save_all(blind_save b);
static inline int state_save_all(secret_save, blind_save b)
{
	return state_save_all(b);
}
int state_restore_all(int in_game, std::nullptr_t, blind_save);
static inline int state_restore_all(int in_game, secret_restore, std::nullptr_t, blind_save blind)
{
	return state_restore_all(in_game, nullptr, blind);
}
void StartNewLevelSub(int level_num, int page_in_textures);
// Actually does the work to start new level
static inline void StartNewLevelSub(int level_num, int page_in_textures, secret_restore)
{
	StartNewLevelSub(level_num, page_in_textures);
}
void init_player_stats_level();
static inline void init_player_stats_level(secret_restore)
{
	init_player_stats_level();
}
#elif defined(DXX_BUILD_DESCENT_II)
int state_restore_all_sub(const char *filename, secret_restore);
void set_pos_from_return_segment(void);
int state_save_all(secret_save, blind_save);
int state_restore_all(int in_game, secret_restore, const char *filename_override, blind_save);
void StartNewLevelSub(int level_num, int page_in_textures, secret_restore);
void init_player_stats_level(secret_restore);
#endif

#endif
