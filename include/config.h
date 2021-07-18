/* include/config.h
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
 * $Id: config.h,v 1.32 2006/02/06 02:13:36 bugs Exp $
 */

#ifndef HAVEINC_config
#define HAVEINC_config

/* flags utilisés pour la config */
#define CF_XMODE 		0x1
#define CF_ADMINEXEMPT 		0x2
#define CF_KILLFORFLOOD 	0x4
#define CF_NOKILL       	0x8
#define CF_WELCOME 		0x10
#define CF_PRIVWELCOME 		0x20
#define CF_NOMAIL		0x40
#define CF_NOREG 		0x80
#define CF_PREMIERE 		0x100
#define CF_RESTART		0x200
/* libre 0x400 utilisé avant pour autojoin admin */
#define CF_HOSTHIDING   	0x800
/* libre 0x1000 utilisé avant pour les moderateurs de merde */
/* libre 0x2000 utilisé avant pour les HALFOPS */
#define CF_HAVE_CRYPTHOST	0x4000
#define CF_CRYPT_MD2_MD5	0x8000
#define CF_WEBSERV		0x10000
#define CF_ADMINREG		0x20000
#define CF_ADMINREGONLY		0x40000
#define CF_NICKSERV		0x80000
#define CF_MEMOSERV		0x100000
#define CF_WELCOMESERV		0x200000
#define CF_VOTESERV		0x400000
#define CF_TRACKSERV		0x800000
#define CF_USERNAME		0x1000000
#define CF_ONLYWEBREG		0x2000000
#define CF_SHA2			0x4000000

#define EndOfTab(x) (x && *x == '}' && (*(x+1) == '\0' || *(x+1) == '\n' || *(x+1) == '\r'))

extern int kill_interval;
extern int ignoretime;
extern int cf_maxlastseen;
extern int cf_chanmaxidle;
extern int cf_limit_update;
extern int cf_warn_purge_delay;
extern int cf_register_timeout;
extern int cf_unreg_reg_delay;
extern int MAXMEMOS;
extern int MAXALIAS;
extern char *cf_quit_msg;
extern char *pasdeperm;
extern char *mailprog;
extern char *scanmsg;
extern char *defraison;
extern char hidden_host[];
extern char *webaddr;
extern int load_config(const char *);
extern char *tmpl_oubli;
extern char *tmpl_register;
extern char *tmpl_memo;

#endif /*HAVEINC_config*/

