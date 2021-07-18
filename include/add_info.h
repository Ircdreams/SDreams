/* include/add_info.h - Ajout d'informations en mémoire
 * Copyright (C) 2004 ircdreams.org
 *
 * contact: bugs@ircdreams.org
 * site web: http://www.ircdreams.org
 *
 * Services pour serveur IRC. Supporté sur IrcDreams V.2
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
 * $Id: add_info.h,v 1.13 2006/01/06 23:12:59 bugs Exp $
 */

#ifndef HAVEINC_add_info
#define HAVEINC_add_info

extern void add_memo(anUser *, const char *, time_t, const char *, int);
extern void add_killinfo(aNick *, enum TimerType);

extern int add_server(const char *, const char *, const char *, const char *, const char *);
extern aDNR *add_dnr(const char *, const char *, const char *, time_t, unsigned int);
extern anAccess *add_access(anUser *, const char *, int, int, time_t);

extern void add_join(aNick *, const char *, int, time_t, aNChan *);
extern void add_ban(aChan *, const char *, const char *, const char *, time_t, time_t, int);

extern void add_alias(anUser *, const char *);

#endif /*HAVEINC_add_info*/
