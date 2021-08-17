/* include/welcome.h
 *
 * Copyright (C) 2002-2005 David Cortier  <Cesar@ircube.org>
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
 * $Id: welcome.h,v 1.5 2005/12/03 14:34:18 romexzf Exp $
 */

#ifndef HAVEINC_welcome
#define HAVEINC_welcome

#define WELCOME_FILE DBDIR"/welcome.db"

struct welcomeinfo {
	char msg[250];
	int id;
	int view;
	struct welcomeinfo *next;
};

extern void write_welcome(void);
extern int global_welcome(aNick *, aChan *, int, char **);
extern void choose_welcome(const char *);
extern void load_welcome(void);

#endif /*HAVEINC_welcome*/
