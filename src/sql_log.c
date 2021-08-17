/* src/sql_log.c - Logs commands into a SQL base
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
 * $Id: sql_log.c,v 1.25 2007/12/08 17:07:03 romexzf Exp $
 */

#include "main.h"
#ifdef SQLLOG
#include "sql_log.h"
#include "mylog.h"
#include "cs_cmds.h"
#include "timers.h"
#include <mysql.h>
#include <errmsg.h>

#define SQL_UINITQ  "INSERT INTO "SQLUSER" VALUES "
#define SQL_CINITQ  "INSERT INTO "SQLCHAN" VALUES "

static char sql_buf[2][2048]= {SQL_UINITQ, SQL_CINITQ};
static unsigned int sql_count[2];
static time_t SQL_LastFlush[2];

#define SQL_WAIT_FLUSH 1200 /* delay between sql flushes */
#define SQLDequeue() (type == SQL_QRAW || sql_count[type] > sizeof sql_buf[type] - 800)

static int sql_send(const char *buf)
{
 	if(mysql_query(bot.sql_id, buf)) /* an error occured while querying */
 	{
 		unsigned int sql_err = mysql_errno(bot.sql_id);

 		/* Try to handle simple disconnection errors smouthly */
		if(sql_err == CR_SERVER_LOST || sql_err == CR_SERVER_GONE_ERROR)
		{
			if(!sql_init(0)) /* reconnect failed, try later */
				log_write(LOG_MAIN, LOG_DOWALLOPS, "DB SQL inaccessible -- nouvel essai"
					" lors du prochain query.");

			else /* yeah ! */
			{
				log_write(LOG_MAIN, LOG_DOWALLOPS, "DB SQL perdue -- reconnexion OK"
					" -- envoi du query.");
				mysql_query(bot.sql_id, buf);
				return 1;
			}
		}
		else log_write(LOG_MAIN, LOG_DOWALLOPS, "SQL Server reports: %s",
				mysql_error(bot.sql_id));

		log_write(LOG_MAIN, LOG_DOWALLOPS, "sql_query failed (%s)", buf);
		return -1;
	}
	return 1;
}

int sql_flush(enum SQL_QType type)
{
	SQL_LastFlush[type] = CurrentTS;
	if(sql_count[type]) sql_send(sql_buf[type]), sql_count[type] = 0;
	return 0;
}

int sql_query(enum SQL_QType type, const char *pattern, ...)
{
	static char raw[1100];
 	register char *p, t;
 	va_list vl;

	if(type != SQL_QRAW)
	{
		if(!sql_count[type]) /* empty query, move after init string */
			sql_count[type] = (type == SQL_QINSERTU ? sizeof(SQL_UINITQ) : sizeof(SQL_CINITQ)) -1;
		else sql_buf[type][sql_count[type]++] = ',';
		p = sql_buf[type] + sql_count[type];
	}
	else p = raw;

	va_start(vl, pattern);
	while((t = *pattern++)) /* %sa (t = %, *pattern = s) */
 	{
 		if(t == '%')
 		{
 			t = *pattern++; /* on drop le formateur (t = s, *pattern = a) */
 			if(t == 's')
 			{
 				register char *tmps = va_arg(vl, char *);
 				if(tmps)
 					while(*tmps) /* copie de la string */
 					{
 						if(*tmps == '\\' || *tmps == '\'' || *tmps == '"' || *tmps == '%')
 							*p++ = '\\'; /* escape it */
 						*p++ = *tmps++;
 					}
 				continue;
 			}
 			if(t == 'c')
 			{
 				*p++ = (char) va_arg(vl, int);
 				continue;
 			}
 			if(t == 'T')
 			{
 				unsigned long tmpi = va_arg(vl, unsigned long);
 				int pos = 31;
 				char bufi[32];

 				if(tmpi == 0)
 				{
 					*p++ = '0';
 					continue;
 				}
 				while(tmpi)	/* on converti une int en base 10 en string */
 				{			/* écriture dans l'ordre inverse 51 > '   1' > '  51'  */
 					bufi[pos--] = '0' + (tmpi % 10);
 					tmpi /= 10;
 				}
 				while(pos < 31) *p++ = bufi[++pos];
 				continue;
			}
 			if(t != '%')
 			{	/* on sous traite la suite à vsnprintf */
 				p += vsprintf(p, pattern - 2, vl); /* on remet le %format */
 				break;
 			}
 		}
 		*p++ = t;
	}
 	*p = '\0';
	va_end(vl);

	if(type == SQL_QRAW) return sql_send(raw);
	else
	{	/* update current length via pointer arithemtic */
		sql_count[type] += (p - &sql_buf[type][sql_count[type]]);
		if(SQLDequeue()) sql_flush(type);
	}
	return 1;
}

static int callback_flush_sql(Timer *timer)
{
	if(CurrentTS - SQL_LastFlush[SQL_QINSERTU] >= SQL_WAIT_FLUSH)
		sql_flush(SQL_QINSERTU);
	if(CurrentTS - SQL_LastFlush[SQL_QINSERTC] >= SQL_WAIT_FLUSH)
		sql_flush(SQL_QINSERTC);

	return 0;
}

int sql_init(int init)
{
	MYSQL_RES *result = NULL;
	int culog = 1, cclog = 1;

 	bot.sql_id = mysql_init(NULL);

 	if(!mysql_real_connect(bot.sql_id, bot.sql_host, bot.sql_user, bot.sql_pass,
 		bot.sql_db, bot.sql_port, NULL, 0))
 	{
		log_write(LOG_MAIN, LOG_DOTTY, "Connexion SQL à %s:%d (user=%s base=%s) impossible.",
			bot.sql_host, bot.sql_port, bot.sql_user, bot.sql_db);
		return 0;
	}

	if(!init) return 1; /* pas d'initialisation à faire */

 	mysql_query(bot.sql_id, "SHOW TABLES"); /* on vérifie si les tables existent */
 	if((result = mysql_store_result(bot.sql_id)))
 	{
 		MYSQL_ROW row;

 		while((row = mysql_fetch_row(result)))
 		{
 			mysql_field_seek(result, 0);
 			if(!strcmp(row[0], SQLUSER)) culog = 0;
 			else if(!strcmp(row[0], SQLCHAN)) cclog = 0;
 		}
 		mysql_free_result(result);
	}
	/* création des tables .. */
 	if(culog && sql_query(SQL_QRAW, "CREATE TABLE "SQLUSER" (user CHAR(%d), TS INT(4), "
 		"cmd CHAR(%d), log CHAR(255))", NICKLEN, CMDLEN) < 0) return 0;
 	if(cclog && sql_query(SQL_QRAW, "CREATE TABLE "SQLCHAN" (user CHAR(%d), chan CHAR(%d), "
 		"TS INT(4), cmd CHAR(%d), log CHAR(255))", NICKLEN, REGCHANLEN, CMDLEN) < 0)
 			return 0;

 	timer_add(SQL_WAIT_FLUSH, TIMER_PERIODIC, callback_flush_sql, NULL, NULL);
	return 1;
}
#endif
