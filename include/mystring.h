/* include/mystring.h - Fonctions de manipulations de strings
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
 * $Id: mystring.h,v 1.2 2006/03/15 06:43:23 bugs Exp $
 */

#ifndef HAVEINC_mystring
#define HAVEINC_mystring

extern int match(const char *, const char *); /* hack*/
extern int mmatch(const char *, const char *); /* hack*/

extern int is_num(const char *);
extern void Strlwr(char *);
extern void strip_newline(char *);

extern int Strtoint(const char *, int *, int, int);
extern int count_char(const char *, int);

extern int split_buf(char *, char **, int);
extern void parv2msgn(int, char **, int, char *, int);
extern char *parv2msg(int, char **, int, int);

extern char *str_dup(char **, const char *);
extern char *Strncpy(char *, const char *, size_t);
extern char *Strtok(char **, char *, int);

extern int fastfmt(char *, const char *, ...);
extern int fastfmtv(char *, const char *, va_list);

extern int myvsnprintf(char *, size_t, const char *, va_list);
extern int mysnprintf(char *, size_t, const char *, ...);

#endif /*HAVEINC_mystring*/
