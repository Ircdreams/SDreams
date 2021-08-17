/* include/data.h
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
 * $Id: data.h,v 1.2 2006/09/13 13:34:17 romexzf Exp $
 */

#ifndef HAVEINC_data
#define HAVEINC_data

extern int data_handle(aData *, const char *, const char *, time_t, int, void *);
extern int data_load(aData **, const char *, const char *, int, time_t, time_t, void *);
extern void data_free(aData *);

#define show_generic(nick, of, type_str, type, f) do { 					\
	if(of->type)														\
	{ 																	\
		char buf[TIMELEN + 1]; 											\
		Strncpy(buf, get_time(nick, of->type->debut), sizeof buf -1); 	\
		csreply(nick, "%c"type_str" par %s %s, expirant %s (%s)", 		\
		 (of->flag & f) ? '\2' : ' ', of->type->from, buf, 				\
		 of->type->expire ? get_time(nick, of->type->expire) : "Jamais",\
		 *of->type->raison ? of->type->raison : "-No Reason-"); 		\
	 } 																	\
 } while(0)

#define show_csuspend(nick, chan) show_generic(nick, chan, "Suspendu", suspend, C_SUSPEND)
#define show_ususpend(nick, user) show_generic(nick, user, "Suspendu", suspend, U_SUSPEND)
#define show_nopurge(nick, user) show_generic(nick, user, "Nopurge", nopurge, U_NOPURGE)
#define show_cantregchan(nick, user) show_generic(nick, user, "CantRegChan", \
										cantregchan, U_CANTREGCHAN)

#endif /*HAVEINC_data*/
