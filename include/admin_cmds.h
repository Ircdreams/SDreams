/* include/admin_cmds.h
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
 * $Id: admin_cmds.h,v 1.8 2006/02/18 07:08:44 bugs Exp $
 */

#ifndef HAVEINC_admincmds
#define HAVEINC_admincmds

extern int inviteme(aNick *, aChan *, int, char **);
extern void CleanUp(void);
extern int die(aNick *, aChan *, int, char **);
extern int restart_bot(aNick *, aChan *, int, char **);
extern int chcomname(aNick *, aChan *, int, char **);
extern int chlevel(aNick *, aChan *, int, char **);
extern int disable_cmd(aNick *, aChan *, int, char **);
extern int globals_cmds(aNick *, aChan *, int, char **);
extern int rehash_conf(aNick *, aChan *, int, char **);
extern int showconfig(aNick *, aChan *, int, char **);

extern int dnrchan_manage(aNick *, aChan *, int, char **); 
extern int dnruser_manage(aNick *, aChan *, int, char **); 
extern int dnr_manage(aNick *, int, int, char **); 

extern int set_motds(aNick *, aChan *, int, char **);

#endif /*admincmds*/
