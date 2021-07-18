/* include/divers.h
 * Copyright (C) 2004 ircdreams.org
 *
 * contact: bugs@ircdreams.org
 * site web: http://ircdreams.org
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
 * $Id: divers.h,v 1.19 2006/03/08 03:12:20 bugs Exp $
 */

#ifndef HAVEINC_divers
#define HAVEINC_divers

extern int uptime(aNick *, aChan *, int, char **);
extern int ctcp_ping(aNick *, aChan *, int, char **);
extern int ctcp_version(aNick *, aChan *, int, char **);
extern int version(aNick *, aChan *, int, char **);
extern int lastseen(aNick *, aChan *, int, char **);
extern int show_admins(aNick *, aChan *, int, char **);
extern int show_helper(aNick *, aChan *, int, char **);
extern int show_ignores(aNick *, aChan *, int, char **);
extern int verify(aNick *, aChan *, int, char **);
extern int swhois(aNick *, aChan *, int , char **);
extern int sethost(aNick *, aChan *, int, char **);
extern int myinfo(aNick *, aChan *, int, char **);
extern int show_country(aNick *, aChan *, int, char **);

#endif /*HAVEINC_divers*/
