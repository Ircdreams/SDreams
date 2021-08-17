/* include/divers.h
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
 * $Id: divers.h,v 1.4 2007/01/02 19:44:30 romexzf Exp $
 */

#ifndef HAVEINC_divers
#define HAVEINC_divers

extern int uptime(aNick *, aChan *, int, char **);
extern int ctcp_ping(aNick *, aChan *, int, char **);
extern int ctcp_version(aNick *, aChan *, int, char **);
extern int lastseen(aNick *, aChan *, int, char **);
extern int show_admins(aNick *, aChan *, int, char **);
extern int verify(aNick *, aChan *, int, char **);

extern void CleanUp(void);

#endif /*HAVEINC_divers*/
