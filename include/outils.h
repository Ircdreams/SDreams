/* include/outils.h - Divers outils
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
 * $Id: outils.h,v 1.35 2008/01/05 01:24:13 romexzf Exp $
 */

#ifndef HAVEINC_outils
#define HAVEINC_outils

#define LISTSEP ','

#define MBUF_NOKEY 		0x1
#define MBUF_NOLIMIT 	0x2

typedef struct MBuf {
	char modes[21];
	char param[KEYLEN + 20]; /* key + limit */
} MBuf;

enum item_type {ITEM_NUMBER, ITEM_INTERVAL};

typedef struct item_opt {
    enum item_type type;
    int min, max;
} item_opt;

extern int is_ip(const char *);

enum getopt_type {GOPT_FLAG = -1, GOPT_STR = 0, GOPT_INT = 1};

extern int getoption(const char *, char **, int, int, enum getopt_type);

extern char *duration(int);
extern time_t convert_duration(const char *);
extern char *get_time(aNick *, time_t);

extern char *GetNUHbyNick(aNick *, int);
extern char *GetPrefix(aNick *);
extern char *GetChanPrefix(aNick *, aJoin *);

extern void base64toip(const char *, struct irc_in_addr *);
extern char *GetUserIP(aNick *, char *);
extern char *GetIP(struct irc_in_addr *, char *);

#define irc_in_addr_is_ipv4(ADDR) (!(ADDR)->in6_16[0] && !(ADDR)->in6_16[1] && !(ADDR)->in6_16[2] \
                                   && !(ADDR)->in6_16[3] && !(ADDR)->in6_16[4] && (ADDR)->in6_16[6] \
                                   && (!(ADDR)->in6_16[5] || (ADDR)->in6_16[5] == 65535))

extern int check_protect(aNick *, aNick *, aChan *);

extern unsigned int parse_umode(unsigned int, const char *);
extern char *GetModes(unsigned int);
extern unsigned int cmodetoflag(unsigned int, const char *);
extern unsigned int string2scmode(struct cmode *, const char *, const char *, const char *);
extern char *GetCModes(struct cmode);
extern void CModes2MBuf(struct cmode *, MBuf *, unsigned int);
extern void modes_reverse(aNChan *, const char *, const char *, const char *);

extern int IsValidNick(const char *);
extern int IsValidMail(const char *);

extern anUser *ParseNickOrUser(aNick *, const char *);

extern int switch_option(aNick *, const char *, const char *, const char *, int *, int);

extern int cswall(const char *, ...);

extern int item_isinlist(item_opt *, const int, const int);
extern int item_parselist(char *, item_opt *, int, int *);

#endif /*HAVEINC_outils*/
