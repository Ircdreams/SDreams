/* include/add_info.h - Ajout d'informations en mémoire
 *
 * Copyright (C) 2002-2007 David Cortier  <Cesar@ircube.org>
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
 * $Id: add_info.h,v 1.10 2007/12/16 20:48:15 romexzf Exp $
 */

#ifndef HAVEINC_add_info
#define HAVEINC_add_info

#ifdef USE_MEMOSERV
extern void add_memo(anUser *, const char *, time_t, const char *, int);
#endif
#ifdef USE_NICKSERV
extern void add_killinfo(aNick *, enum TimerType);
#endif

extern int add_server(const char *, const char *, const char *, const char *, const char *);
extern anAccess *add_access(anUser *, const char *, int, int, time_t);
extern void add_join(aNick *, const char *, int, time_t, aNChan *);
extern void ban_load(aChan *, const char *, const char *, const char *, time_t, time_t, int);
extern aBan *ban_create(const char *, const char *, const char *, time_t, time_t, int);
extern void ban_add(aChan *, aBan *);


#endif /*HAVEINC_add_info*/
