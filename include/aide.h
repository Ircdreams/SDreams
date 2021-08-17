/* include/aide.h
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
 * $Id: aide.h,v 1.3 2004/08/24 15:19:10 romexzf Exp $
 */

#ifndef HAVEINC_aide
#define HAVEINC_aide

extern int aide(aNick *, aChan *, int, char **);
extern int syntax_cmd(aNick *, aHashCmd *);
extern int help_load(Lang *);
extern int help_cmds_realloc(int);

#endif /*HAVEINC_aide*/
