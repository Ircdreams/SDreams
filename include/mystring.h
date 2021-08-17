/* include/mystring.h - Fonctions de manipulations de strings
 *
 * Copyright (C) 2002-2008 David Cortier  <Cesar@ircube.org>
 *                         Romain Bignon  <Progs@coderz.info>
 *                         Benjamin Beret <kouak@kouak.org>
 *
 * site web: http://sf.net/projects/scoderz/
 *
 * Services pour serveur IRC. Supporté sur IRCoderz
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
 * $Id: mystring.h,v 1.4 2008/01/20 13:50:39 romexzf Exp $
 */

#ifndef HAVEINC_mystring
#define HAVEINC_mystring

extern int match(const char *, const char *); /* hack */
extern int mmatch(const char *, const char *); /* hack */

extern int is_num(const char *);
extern void strupr(char *);
extern void strlwr(char *);
extern void strip_newline(char *);

extern int Strtoint(const char *, int *, int, int);
extern int count_char(const char *, int);
extern int HasWildCard(const char *);

extern int split_buf(char *, char **, int);
extern void parv2msgn(int, char **, int, char *, size_t);
extern char *parv2msg(int, char **, int, size_t);

extern char *str_dup(char **, const char *);
extern char *Strncpy(char *, const char *, size_t);
extern char *Strtok(char **, char *, int);

extern size_t fastfmt(char *, const char *, ...);
extern size_t fastfmtv(char *, const char *, va_list);

extern size_t myvsnprintf(char *, size_t, const char *, va_list);
extern size_t mysnprintf(char *, size_t, const char *, ...);

#endif /*HAVEINC_mystring*/
