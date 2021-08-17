/* include/debug.h
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
 * $Id: debug.h,v 1.1 2003/12/12 17:44:58 romexzf Exp $
 */

#ifndef HAVEINC_debug
#define HAVEINC_debug

/* flags de debug */
#define W_WARN 		0x01 /* wallops envoyé*/
#define W_PROTO 	0x02 /* erreur de protocole P10 > ce n'est pas notre faute..*/
#define W_DESYNCH 	0x04 /* ARG on est desynchronisé.. */
#define W_MAX 		0x08 /* BOUM erreure majeure, malloc ou du genre, crash à suivre*/
#define W_TTY 		0x10 /* envoyé en console (lancement)*/

extern int Debug(int, const char *, ...);

#endif /*HAVEINC_debug*/
