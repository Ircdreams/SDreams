/* include/config.h
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
 * $Id: config.h,v 1.12 2007/01/02 19:44:30 romexzf Exp $
 */

#ifndef HAVEINC_config
#define HAVEINC_config

extern int ConfFlag;

#define GetConf(x) 				(ConfFlag & (x))

/* flags utilisés pour la config */

#define CF_XMODE 		0x001
#define CF_ADMINEXEMPT 	0x002
#define CF_KILLFORFLOOD 0x004
#define CF_NOKILL 		0x008
#define CF_HOSTHIDING 	0x010
#define CF_WELCOME 		0x020
#define CF_PRIVWELCOME 	0x040
#define CF_NOMAIL	 	0x080
#define CF_NOREG 		0x100
#define CF_PREMIERE 	0x200
#define CF_RESTART 		0x400

extern int cf_kill_interval;
extern int cf_ignoretime;
extern int cf_maxlastseen;
extern int cf_chanmaxidle;
extern int cf_write_delay;
extern int cf_limit_update;
extern int cf_warn_purge_delay;
extern int cf_register_timeout;
extern int cf_unreg_reg_delay;
extern char *cf_quit_msg;
extern char *cf_pasdeperm;
extern char *cf_mailprog;
extern char cf_hidden_host[];
extern char cf_defraison[];

extern int load_config(const char *);

#endif /*HAVEINC_config*/
