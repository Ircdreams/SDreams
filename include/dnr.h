/* include/dnr.h - Gestion des DNR (Do Not Register)
 *
 * Copyright (C) 2002-2007 David Cortier  <Cesar@ircube.org>
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
 * $Id: dnr.h,v 1.2 2007/12/02 17:01:09 romexzf Exp $
 */


#ifndef HAVEINC_dnr
#define HAVEINC_dnr

typedef struct aDNR_ {
	char *mask;
	char *raison;
	struct aDNR_ *next;
	struct aDNR_ *last;
	unsigned int flag;

#define DNR_TYPEUSER 	0x01
#define DNR_TYPECHAN 	0x02
#define DNR_MASK 		0x04

#define DNR_TYPES 		(DNR_TYPEUSER|DNR_TYPECHAN)

	time_t date;
	char from[NICKLEN + 1];
} aDNR;

extern aDNR *dnr_find(const char *, int);

#define IsBadNick(nick) 		dnr_find((nick), DNR_TYPEUSER|DNR_MASK)
#define IsBadChan(chan) 		dnr_find((chan), DNR_TYPECHAN|DNR_MASK)

extern int dnrchan_manage(aNick *, aChan *, int, char **);
extern int dnruser_manage(aNick *, aChan *, int, char **);

extern int load_dnr(int);
extern void write_dnr(void);

extern void dnr_clean(void);

#endif /*dnr*/
