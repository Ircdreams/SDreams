/* include/admin_cmds.h
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
 * $Id: admin_cmds.h,v 1.4 2007/01/02 19:44:30 romexzf Exp $
 */

#ifndef HAVEINC_admincmds
#define HAVEINC_admincmds

extern int inviteme(aNick *, aChan *, int, char **);
extern int die(aNick *, aChan *, int, char **);
extern int restart_bot(aNick *, aChan *, int, char **);
extern int chcomname(aNick *, aChan *, int, char **);
extern int chlevel(aNick *, aChan *, int, char **);
extern int disable_cmd(aNick *, aChan *, int, char **);
extern int globals_cmds(aNick *, aChan *, int, char **);
extern int rehash_conf(aNick *, aChan *, int, char **);
extern int showconfig(aNick *, aChan *, int, char **);

#ifdef USE_WELCOMESERV
extern int set_motds(aNick *, aChan *, int, char **);
#endif

#endif /*admincmds*/
