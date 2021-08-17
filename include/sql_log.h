/* include/sql_log.h
 * Copyright (C) 2002-2004 Inter System
 *
 * contact: Progs@Inter-System.Net
 *          Cesar@Inter-System.Net
 *          kouak@kouak.org
 * site web: http://coderz.inter-system.net
 *
 * Services pour serveur IRC. Supporté sur IrcProgs et IrCoderZ
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
 * $Id: sql_log.h,v 1.6 2005/04/23 22:42:10 romexzf Exp $
 */

#ifndef HAVEINC_sqllog
#define HAVEINC_sqllog

#define SQLUSER "usercmdslog"
#define SQLCHAN "chancmdslog"

enum SQL_QType {SQL_QINSERTU = 0, SQL_QINSERTC, SQL_QRAW};

extern int sql_query(enum SQL_QType, const char *, ...);
extern int sql_init(int);
extern int sql_flush(enum SQL_QType);

#endif /*HAVEINC_sqllog*/
