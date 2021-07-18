/* include/ban.h
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
 * $Id: ban.h,v 1.5 2004/11/08 14:51:18 bugs Exp $
 */

#ifndef HAVEINC_ban
#define HAVEINC_ban

extern int ban_cmd(aNick *, aChan *, int, char **);
extern int kickban(aNick *, aChan *, int, char **);
extern int banlist(aNick *, aChan *, int, char **);
extern int clear_bans(aNick *, aChan *, int, char **);
extern int unban(aNick *, aChan *, int, char **);
extern int unbanme(aNick *, aChan *, int, char **);
extern char *getbanmask(aNick *, int);
extern aBan *is_ban(aNick *, aChan *, aBan *);
extern char *GetBanType(aChan *);

#endif /*HAVEINC_ban*/
