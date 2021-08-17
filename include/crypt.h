/* include/crypt.h - Password hashing stuff
 *
 * Copyright (C) 2002-2006 David Cortier  <Cesar@ircube.org>
 *                         Romain Bignon  <Progs@kouak.org>
 *                         Benjamin Beret <kouak@kouak.org>
 *
 * site web: http://sf.net/projects/scoderz/
 *
 * Services pour serveur IRC. Supporté par IRCoderz
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
 * $Id: crypt.h,v 1.5 2006/04/09 19:15:34 romexzf Exp $
 */

#ifndef HAVEINC_crypt
#define HAVEINC_crypt

#ifdef MD5TRANSITION
#	define cryptpass(x) crypt((x), "la")

extern char *crypt(const char *, const char *);

#endif

#define PWD_HASHED 0x1 /* Password already hashed */

extern char *MD5pass(const char *, char *);
extern int checkpass(const char *, anUser *);

extern char *create_password(const char *);
extern char *create_cookie(anUser *);
extern int password_update(anUser *, const char *, int);

#endif /* HAVEINC_crypt */
