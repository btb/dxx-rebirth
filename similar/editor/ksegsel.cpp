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
 * Functions for selecting segments
 *
 */

#include <utility>
#include <string.h>

#include "inferno.h"
#include "editor/editor.h"
#include "editor/esegment.h"
#include "editor/medmisc.h"
#include "gameseg.h"
#include "kdefs.h"

__attribute_warn_unused_result
static vsegptridx_t get_any_attached_segment(const vsegptridx_t curseg_num, const uint_fast32_t skipside)
{
	for (uint_fast32_t s = 0; s != MAX_SIDES_PER_SEGMENT; ++s)
	{
		if (unlikely(s == skipside))
			continue;
		const auto child = curseg_num->children[s];
		if (IS_CHILD(child))
			return vsegptridx(child);
	}
	return curseg_num;
}

__attribute_warn_unused_result
static vsegptridx_t get_previous_segment(const vsegptridx_t curseg_num, const uint_fast32_t curside)
{
	const auto side_child = curseg_num->children[Side_opposite[curside]];
	if (IS_CHILD(side_child))
		return vsegptridx(side_child);
	// no segment on opposite face, connect to anything
	return get_any_attached_segment(curseg_num, curside);
}

// ---------------------------------------------------------------------------------------
// Select previous segment.
//	If there is a connection on the side opposite to the current side, then choose that segment.
// If there is no connecting segment on the opposite face, try any segment.
__attribute_warn_unused_result
static std::pair<vsegptridx_t, uint_fast32_t> get_previous_segment_side(const vsegptridx_t curseg_num, const uint_fast32_t curside)
{
	const auto &newseg_num = get_previous_segment(curseg_num, curside);
	// Now make Curside point at the segment we just left (unless we couldn't leave it).
	return {newseg_num, newseg_num == curseg_num ? curside : find_connect_side(curseg_num, newseg_num)};
}

// --------------------------------------------------------------------------------------
// Select next segment.
//	If there is a connection on the current side, then choose that segment.
// If there is no connecting segment on the current side, try any segment.
__attribute_warn_unused_result
static std::pair<vsegptridx_t, uint_fast32_t> get_next_segment_side(const vsegptridx_t curseg_num, uint_fast32_t curside)
{
	const auto side_child = curseg_num->children[curside];
	if (IS_CHILD(side_child))
	{
		const auto newseg_num = vsegptridx(side_child);
		// Find out what side we came in through and favor side opposite that
		const auto newside = Side_opposite[find_connect_side(curseg_num, newseg_num)];
		// If there is nothing attached on the side opposite to what we came in (*newside), pick any other side
		if (!IS_CHILD(newseg_num->children[newside]))
			for (uint_fast32_t s = 0; s != MAX_SIDES_PER_SEGMENT; ++s)
			{
				const auto cseg = newseg_num->children[s];
				if (cseg != curseg_num && IS_CHILD(cseg))
					return {newseg_num, s};
			}
		return {newseg_num, newside};
	}
	else
	{
		return {curseg_num, curside};
	}

}

// ---------- select current segment ----------
int SelectCurrentSegForward()
{
	const auto p = get_next_segment_side(Cursegp,Curside);
	const auto &newseg_num = p.first;

	if (newseg_num != Cursegp)
	{
		Cursegp = newseg_num;
		const auto &newside = p.second;
		Curside = newside;
		Update_flags |= UF_ED_STATE_CHANGED;
		if (Lock_view_to_cursegp)
			set_view_target_from_segment(Cursegp);

		med_create_new_segment_from_cursegp();
		mine_changed = 1;
	}

	return 1;
}

// -------------------------------------------------------------------------------------
int SelectCurrentSegBackward()
{
	const auto &p = get_previous_segment_side(Cursegp,Curside);
	Cursegp = p.first;
	Curside = p.second;

	if (Lock_view_to_cursegp)
		set_view_target_from_segment(Cursegp);
	Update_flags |= UF_ED_STATE_CHANGED;
	mine_changed = 1;
	med_create_new_segment_from_cursegp();

	return 1;
}


// ---------- select next/previous side on current segment ----------
int SelectNextSide()
{
	if (++Curside >= MAX_SIDES_PER_SEGMENT)
		Curside = 0;
	Update_flags |= UF_ED_STATE_CHANGED;
	mine_changed = 1;
	return 1;
}

int SelectPrevSide()
{
	if (--Curside < 0)
		Curside = MAX_SIDES_PER_SEGMENT-1;
	Update_flags |= UF_ED_STATE_CHANGED;
	mine_changed = 1;
	return 1;
}

//  ---------- Copy current segment and side to marked segment and side ----------

int CopySegToMarked()
{
   autosave_mine(mine_filename);
	undo_status[Autosave_count] = "Mark Segment UNDONE.";
	Markedsegp = Cursegp;
	Markedside = Curside;
	Update_flags |= UF_ED_STATE_CHANGED;
	mine_changed = 1;
	return 1;
}

// ---------- select absolute face on segment ----------

int SelectBottom()
{
	Curside = WBOTTOM;
	Update_flags |= UF_ED_STATE_CHANGED;
	mine_changed = 1;
	return 1;
}

int SelectFront()
{
	Curside = WFRONT;
	Update_flags |= UF_ED_STATE_CHANGED;
	mine_changed = 1;
	return 1;
}

int SelectTop()
{
	Curside = WTOP;
	Update_flags |= UF_ED_STATE_CHANGED;
	mine_changed = 1;
	return 1;
}

int SelectBack()
{
	Curside = WBACK;
	Update_flags |= UF_ED_STATE_CHANGED;
	mine_changed = 1;
	return 1;
}

int SelectLeft()
{
	Curside = WLEFT;
	Update_flags |= UF_ED_STATE_CHANGED;
	mine_changed = 1;
	return 1;
}

int SelectRight()
{
	Curside = WRIGHT;
	Update_flags |= UF_ED_STATE_CHANGED;
	mine_changed = 1;
	return 1;
}

