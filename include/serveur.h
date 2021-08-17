/* include/serveur.h - sock & irc
 *
 * Copyright (C) 2002-2005 David Cortier  <Cesar@ircube.org>
 *                         Romain Bignon  <Progs@ir3.org>
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
 * $Id: serveur.h,v 1.23 2005/11/02 16:56:16 romexzf Exp $
 */

#ifndef HAVEINC_serveur
#define HAVEINC_serveur

extern aServer *mainhub;

#define GetInfobyPrefix(x) num2nickinfo(x)

extern int m_clearmode(int, char **);
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
#ifdef HAVE_OPLEVELS
extern int m_destruct(int, char **);
#endif

#endif /*HAVEINC_serveur*/
