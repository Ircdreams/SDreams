/* include/cs_cmds.h commandes IRC du CS
 * Copyright (C) 2004-2006 ircdreams.org
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
 * $Id: cs_cmds.h,v 1.15 2006/02/16 17:36:26 bugs Exp $
 */

#ifndef HAVEINC_cs_cmds
#define HAVEINC_cs_cmds

#include "token.h"

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
extern void osmode(aChan *, int, const char *, ...);

#endif /*HAVEINC_cs_cmds*/
