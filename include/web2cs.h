/* include/web2cs.h - Déclarations du Web2CS pour le core
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
 * $Id: web2cs.h,v 1.31 2006/12/20 23:36:56 romexzf Exp $
 */

#ifndef HAVEINC_web2cs
#define HAVEINC_web2cs

#ifdef WEB2CS

typedef struct w2c_hallow {
	unsigned long int host;
	struct w2c_hallow *next;
} w2c_hallow;


/* dear global vars */

extern int bind_sock;

extern w2c_hallow *w2c_hallowhead;

extern int w2c_handle_read(int);
extern void w2c_initsocket(void);
extern int w2c_addclient(int, struct in_addr *);

#endif
#endif
