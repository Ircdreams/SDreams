/* include/cs_cmds.h commandes IRC du CS
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
 * $Id: cs_cmds.h,v 1.6 2006/02/08 16:12:53 romexzf Exp $
 */

#ifndef HAVEINC_cs_cmds
#define HAVEINC_cs_cmds

#include "token.h"

#define csntc csreply

extern int csreply(aNick *, const char *, ...);

extern void putserv(const char *, ...);
extern void putchan(const char *);
extern void csmode(aChan *, int, const char *, ...);
extern void cstopic(aChan *, const char *);
extern void cskick(const char *, const char *, const char *, ...);
extern void cswallops(const char *, ...);
extern void csjoin(aChan *, int);
extern void cspart(aChan *, const char *);
extern void cs_account(aNick *, anUser *);

#endif /*HAVEINC_cs_cmds*/
