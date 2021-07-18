/* 
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
 * $Id: crypt.h,v 1.7 2005/06/12 19:13:31 bugs Exp $
 */

#ifndef HAVEINC_crypt
#define HAVEINC_crypt

#define cryptpass(x) crypt((x), "la")

extern char *crypt(const char *, const char *);

extern char *MD5pass(const char *, char *);
extern int checkpass(const char *, anUser *);

extern char *create_password(const char *);

#endif /* HAVEINC_crypt */
