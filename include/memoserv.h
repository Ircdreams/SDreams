/* include/memoserv.h
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
 * $Id: memoserv.h,v 1.10 2006/03/03 14:34:54 bugs Exp $
 */

#ifndef HAVEINC_memoserv
#define HAVEINC_memoserv

#define MemoRead(x)     ((x)->flag & MEMO_READ)
#define WARNMEMO	50

extern void show_notes(aNick *);
extern int memo_del(anUser *, int *, int);
extern int memos(aNick *, aChan *, int, char **);
extern int memo_parselist(char *list, int *notes, const int size);
extern int memo_isinlist(const int *list, const int max, const int index);
extern int chanmemo(aNick*, aChan *, int, char **); 

#endif /*HAVEINC_memoserv*/
