/* include/chanserv.h - commandes pour gerer les salons
 * Copyright (C) 2002-2003 Inter System
 *
 * contact: Progs@Inter-System.Net
 *          Cesar@Inter-System.Net
 *          kouak@kouak.org
 * site web: http://coderz.inter-system.net
 *
 * Services pour serveur IRC. Supporté sur IrcProgs et IrCoderZ
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
 */

#ifndef HAVEINC_chanserv
#define HAVEINC_chanserv

#define MAXACCESSMATCHES 10

extern int show_access(aNick *, aChan *, int, char **);
extern void show_accessn(anAccess *, anUser *, aNick *);
extern int add_user(aNick *, aChan *, int, char **);
extern int del_user(aNick *, aChan *, int, char **);
extern int kick(aNick *, aChan *, int, char **);
extern int mode(aNick *, aChan *, int, char **);
extern int topic(aNick *, aChan *, int, char **);
extern int rdeftopic(aNick *, aChan *, int, char **);
extern int rdefmodes(aNick *, aChan *, int, char **);
extern int info(aNick *, aChan *, int, char **);
extern int invite(aNick *, aChan *, int, char **);
extern int clearmodes(aNick *, aChan *, int, char **);
extern int see_alist(aNick *, aChan *, int, char **);
extern int admin_say(aNick *, aChan *, int, char **);
extern int admin_do(aNick *, aChan *, int, char **);

#endif /*HAVEINC_chanserv*/
