/* include/opdeop.h
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
 * $Id: opdeop.h,v 1.1 2003/12/12 17:44:58 romexzf Exp $
 */

#ifndef HAVEINC_opdeop
#define HAVEINC_opdeop

#define NUMLEN (2*(NUMSERV+1))

extern int voice(aNick *, aChan *, int, char **);
extern int devoice(aNick *, aChan *, int, char **);
extern int deop(aNick *, aChan *, int, char **);
extern int op(aNick *, aChan *, int, char **);
extern int devoiceall(aNick *, aChan *, int, char **);
extern int voiceall(aNick *, aChan *, int, char **);
extern int deopall(aNick *, aChan *, int, char **);
extern int opall(aNick *, aChan *, int, char **);

#endif /*HAVEINC_opdeop*/
