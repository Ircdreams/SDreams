/* include/outils.h - Divers outils
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
 * $Id: outils.h,v 1.30 2006/02/28 06:36:46 bugs Exp $
 */

#ifndef HAVEINC_outils
#define HAVEINC_outils

#define LISTSEP ','

#define MBUF_NOKEY      0x1 
#define MBUF_NOLIMIT    0x2 
    
typedef struct MBuf { 
        char modes[21]; 
        char param[KEYLEN + 20]; /* key + limit */ 
} MBuf; 

extern int is_ip(const char *);

extern int getoption(const char *, char **, int, int, int);

extern char *duration(int);
extern time_t convert_duration(const char *);
extern char *get_time(aNick *, time_t);

extern char *GetNUHbyNick(aNick *, int);
extern char *GetPrefix(aNick *);
extern char *GetChanPrefix(aNick *, aJoin *);
extern char *GetIP(const char *);

extern void putlog(const char *, const char *, ...);
extern int check_protect(aNick *, aNick *, aChan *);

extern int parse_umode(int, const char *);
extern char *GetModes(int);
extern int cmodetoflag(int, const char *);
extern int string2scmode(struct cmode *, const char *, const char *, const char *);
extern char *GetCModes(struct cmode);
extern void CModes2MBuf(struct cmode *, MBuf *, int);
extern void modes_reverse(aNChan *, const char *, const char *, const char *);

extern int IsValidNick(char *);
extern int IsValidMail(char *);

extern anUser *ParseNickOrUser(aNick *, char *);

extern int switch_option(aNick *, const char *, const char *, const char *, int *, int);

extern int cswall(const char *, ...);
extern int IsValidHost(char *);

extern int item_isinlist(const int *, const int, const int); 
extern int item_parselist(char *, int *, const int);
extern char *pretty_mask(char *);

extern void buildmymotd();

#endif /*HAVEINC_outils*/
