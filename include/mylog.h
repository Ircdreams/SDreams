/* include/mylog.h - Log system
 *
 * Copyright (C) 2002-2007 David Cortier  <Cesar@ircube.org>
 *                         Romain Bignon  <Progs@coderz.info>
 *                         Benjamin Beret <kouak@kouak.org>
 *
 * SDreams v2 (C) 2021 -- Ext by @bugsounet <bugsounet@bugsounet.fr>
 * site web: http://www.ircdreams.org
 *
 * Services pour serveur IRC. Supporté sur Ircdreams v3
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
 * $Id: mylog.h,v 1.2 2007/12/01 02:22:31 romexzf Exp $
 */

#ifndef HAVEINC_mylog
#define HAVEINC_mylog

enum LogType {LOG_MAIN = 0, LOG_SOCKET, LOG_DB, LOG_RAW,
			LOG_UCMD, LOG_CCMD,

#ifdef HAVE_VOTE
			LOG_VOTE,
#endif
#ifdef WEB2CS
			LOG_W2C, LOG_W2CCMD, LOG_W2CRAW,
#endif
			LOG_TYPE_COUNT};

#define LOG_DOWALLOPS 		0x01
#define LOG_DONTLOG 		0x02
#define LOG_DONTFLUSH 		0x04
#define LOG_DOTTY 			0x08
#define LOG_DOLOG 			0x10

extern void log_clean(int);
extern void log_conf_handler(const char *, char *);

extern int log_need_action(enum LogType);

extern int log_write(enum LogType, unsigned int, const char *, ...);
extern int log_writev(enum LogType, unsigned int, const char *, va_list);

extern int log_fd_in_use(int);

#endif /*mylog*/
