/* include/ban.h
 *
 * Copyright (C) 2002-2008 David Cortier  <Cesar@ircube.org>
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
 * $Id: ban.h,v 1.4 2008/01/04 13:21:33 romexzf Exp $
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
extern const char *GetBanType(aChan *);


extern int ipmask_check(const struct irc_in_addr *, const struct irc_in_addr *, unsigned char );
extern int ipmask_parse(const char *, struct irc_in_addr *, unsigned char *);
#endif /*HAVEINC_ban*/
