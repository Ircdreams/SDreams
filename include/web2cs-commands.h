/* include/web2cs-commands.h - Déclarations du Web2CS pour web2cs-engine
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
 * $Id: web2cs-commands.h,v 1.1 2006/12/20 23:36:56 romexzf Exp $
 */

#ifndef HAVEINC_web2cs_commands
#define HAVEINC_web2cs_commands

extern int w2c_passwd(WClient *, int, char **);
extern int w2c_login(WClient *, int, char **);
extern int w2c_wauth(WClient *, int, char **);
extern int w2c_wcookie(WClient *, int, char **);
extern int w2c_oubli(WClient *, int, char **);
extern int w2c_channel(WClient *, int, char **);
extern int w2c_user(WClient *, int, char **);
extern int w2c_register(WClient *, int, char **);
extern int w2c_regchan(WClient *, int, char **);
extern int w2c_memo(WClient *, int, char **);
extern int w2c_isreg(WClient *, int, char **);

#endif

