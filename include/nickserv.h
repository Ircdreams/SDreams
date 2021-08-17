/* include/nickserv.ch - Diverses commandes sur le module nickserv
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
 * $Id: nickserv.h,v 1.9 2006/02/14 17:26:40 romexzf Exp $
 */

#ifndef HAVEINC_nickserv
#define HAVEINC_nickserv

extern int user_set(aNick *, aChan *, int, char **);
extern int oubli_pass(aNick *, aChan *, int, char **);
extern int ns_login(aNick *, aChan *, int, char **);
extern int myaccess(aNick *, aChan *, int, char **);
extern int deauth(aNick *, aChan *, int, char **);
extern int recover(aNick *, aChan *, int, char **);

#endif /*HAVEINC_nickserv*/
