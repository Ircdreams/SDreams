/* include/serveur.h - sock & irc
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
 * $Id: serveur.h,v 1.11 2006/01/09 15:00:45 bugs Exp $
 */

#ifndef HAVEINC_serveur
#define HAVEINC_serveur

extern aServer *mainhub;

extern int m_clearmode(int, char **);
#define GetInfobyPrefix(x) num2nickinfo(x)

extern int m_ping(int, char **);
extern int m_privmsg(int, char **);
extern int m_pass(int, char **);
extern int m_kick(int, char **);
extern int m_part(int, char **);
extern int m_join(int, char **);
extern int m_mode(int, char **);
extern int m_eob(int, char **);
extern int m_squit(int, char **);
extern int m_server(int, char **);
extern int m_create(int, char **);
extern int m_nick(int, char **);
extern int m_quit(int, char **);
extern int m_burst(int, char **);
extern int m_away(int, char **);
extern int m_whois(int, char **);
extern int m_kill(int, char **);
extern int m_topic(int, char **);
extern int m_error(int, char **);
extern int m_account(int, char **);
extern int m_svshost(int, char **);
extern int r_motd(int, char **);

#endif /*HAVEINC_serveur*/
