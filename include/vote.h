/* include/vote.h
 *
 * Copyright (C) 2002-2006 David Cortier  <Cesar@ircube.org>
 *                         Romain Bignon  <Progs@kouak.org>
 *                         Benjamin Beret <kouak@kouak.org>
 *
 * site web: http://sf.net/projects/scoderz/
 *
 * Services pour serveur IRC. Supporté sur IrcProgs et IRCoderz
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
 * $Id: vote.h,v 1.9 2006/08/11 23:22:01 romexzf Exp $
 */

#ifndef HAVEINC_vote
#define HAVEINC_vote

#define MAXVOTEPROP 20
#define VOTE_FILE DBDIR"/votes.db"

#define CanVote(user) 	(Vote.actif && (user)->level >= Vote.level \
							&& (user)->reg_time <= Vote.start_time \
							&& !UVote(user) && !UNoVote(user))

struct votes {
	char prop[MEMOLEN + 1];
	int voies;
};

struct Vote {
	int votes;
	int actif;
	int level;
	int nbprop;
	time_t start_time;
	Timer *timer_end;
};

extern struct votes vote[];
extern struct Vote Vote;

extern void write_votes(void);
extern void load_votes(void);
extern void show_vote(aNick *);
extern int vote_results(aNick *, aChan *, int, char **);
extern int voter(aNick *, aChan *, int, char **);
extern int do_vote(aNick *, aChan *, int, char **);

#endif /*HAVEINC_vote*/
