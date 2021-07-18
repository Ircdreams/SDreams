/* include/cs_register.h
 * Copyright (C) 2004-2006 ircdreams.org
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
 * $Id: cs_register.h,v 1.4 2006/02/16 17:36:26 bugs Exp $
 */

#ifndef HAVEINC_cs_register
#define HAVEINC_cs_register

extern int chan_check(const char *, aNick *);

int register_chan(aNick *, aChan *, int, char **);
int ren_chan(aNick *, aChan *, int, char **);
int unreg_chan(aNick *, aChan *, int, char **);

extern int register_user(aNick *, aChan *, int, char **);
extern int drop_user(aNick *, aChan *, int, char **);

#endif /*HAVEINC_cs_register*/
