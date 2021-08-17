/* include/track.h
 *
 * Copyright (C) 2002-2008 David Cortier  <Cesar@ircube.org>
 *                         Romain Bignon  <Progs@coderz.info>
 *                         Benjamin Beret <kouak@kouak.org>
 *
 * site web: http://sf.net/projects/scoderz/
 *
 * Services pour serveur IRC. Supporté sur IRCoderz
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * $Id: track.h,v 1.5 2008/01/05 18:34:13 romexzf Exp $
 */

#ifndef HAVEINC_track
#define HAVEINC_track

struct track {
	aNick *tracker;
	anUser *tracked;
	Timer *timer;
	time_t from, expire;
	struct track *next, *last;
};

extern void track_notify(anUser *, const char *, ...);
extern void track_admin_quit(aNick *);
extern void del_track(struct track *);

extern struct track *istrack(anUser *);
extern int cmd_track(aNick *, aChan *, int, char **);

#endif /*HAVEINC_track*/
