/* src/mylog.c - Système de log
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
 * $Id: mylog.c,v 1.2 2007/12/01 02:22:31 romexzf Exp $
 */

#include "main.h"
#include "config.h"
#include "outils.h"
#include "debug.h"
#include "mylog.h"
#include "cs_cmds.h"
#include "mystring.h"
#include <unistd.h>

#define LOGMAXLEN 2048

 struct LogFile {
	char *filename;
	FILE *file;
	unsigned int used;
	struct LogFile *next, *last;
} *LogFiles = NULL;

static struct LogSys {
	enum LogType type;
	const char *name;

#define LOG_ACTIONS 	(LOG_DOWALLOPS|LOG_DOTTY|LOG_DOLOG)

	unsigned int flag;
	struct LogFile *logfile;
} LogSystems[LOG_TYPE_COUNT] = {
	{LOG_MAIN, 		"main", 	0, NULL},
	{LOG_SOCKET, 	"socket", 	0, NULL},
	{LOG_DB, 		"db", 		0, NULL},
	{LOG_RAW, 		"RAW", 		0, NULL},

#ifdef HAVE_VOTE
	{LOG_VOTE, 		"vote", 	0, NULL},
#endif

#ifdef WEB2CS
	{LOG_W2C, 		"W2C", 		0, NULL},
	{LOG_W2CCMD, 	"W2C_cmd", 	0, NULL},
	{LOG_W2CRAW, 	"W2C_raw", 	0, NULL},
#endif
};

#define logsys_bytype(x) 	(&LogSystems[(x)])

static struct LogSys *log_find_by_name(const char *name)
{
	int i = 0;

	for(; i < LOG_TYPE_COUNT; ++i)
		if(!strcasecmp(name, LogSystems[i].name)) return &LogSystems[i];

	return NULL;
}

static struct LogFile *log_find_logfile(const char *filename)
{
	struct LogFile *log = LogFiles;

	for(; log && strcasecmp(filename, log->filename); log = log->next);

	return log;
}

static struct LogFile *log_new_logfile(const char *filename)
{
	/* Try to find out if this filename is already used by another LogSys */
	struct LogFile *logfile = log_find_logfile(filename);

	if(!logfile) /* Create a new LogFile */
	{
		if(!(logfile = malloc(sizeof *logfile)))
		{
			Debug(W_MAX|W_WARN, "log::newlogfile: OOM for file '%s'", filename);
			return NULL;
		}
		logfile->filename = NULL;
		logfile->file = NULL;
		str_dup(&logfile->filename, filename);
		logfile->used = 1;

		logfile->next = LogFiles;
		logfile->last = NULL;
		if(LogFiles) LogFiles->last = logfile;
		LogFiles = logfile;
	}
	else ++logfile->used;

	/* Try to open file if it's not already */
	if(!logfile->file && !(logfile->file = fopen(logfile->filename, "a")))
		Debug(W_WARN, "log::newlogfile: unable to open file '%s' : %s",
			filename, strerror(errno));

	return logfile;
}

static void log_close_logfile(struct LogFile *logfile)
{
	if(--logfile->used <= 0) /* if parent was the last user of this file */
	{
		/* unlink it */
		if(logfile->next) logfile->next->last = logfile->last;
		if(logfile->last) logfile->last->next = logfile->next;
		else LogFiles = logfile->next;

		/* close & clean up */
		fclose(logfile->file);
		free(logfile->filename);
		free(logfile);
	}
}

static void log_set_flag(struct LogSys *log, unsigned int flag)
{
	log->flag |= flag;
}

static void log_set_file(struct LogSys *log, const char *filename)
{
	/* Close or remove an utilization of an existing logfile */
	if(log->logfile) log_close_logfile(log->logfile);

	log->logfile = log_new_logfile(filename);
	log->flag |= LOG_DOLOG;
}

void log_clean(int reset)
{
	int i = 0;

	for(; i < LOG_TYPE_COUNT; ++i)
	{
		struct LogSys *log = &LogSystems[i];

		if(log->logfile)
		{
			log_close_logfile(log->logfile);
			log->logfile = NULL;
		}
		/* if reset != 0, we need to re-initialize (called from rehash) */
		if(reset) log->flag = 0;
	}
}

void log_conf_handler(const char *typename, char *list)
{
	char *save = NULL;
	struct LogSys *log = log_find_by_name(typename);

	if(!log)
		Debug(W_TTY|W_WARN, "load::config::log::handle(%s): unknown logtype", typename);

	else
	{
		/* Now, parse list to exract correct way of logging */
		for(list = Strtok(&save, list, ' '); list; list = Strtok(&save, NULL, ' '))
		{
			if(!strcmp("wallops", list)) log_set_flag(log, LOG_DOWALLOPS);

			else if(!strcmp("tty", list)) log_set_flag(log, LOG_DOTTY);

			else if(!strncmp("file:", list, 5))
			{
				list += 5;
				if(*list) log_set_file(log, list);
			}

			else if(!strcmp("none", list))
			{
				log->flag = 0;
				if(log->logfile)
				{
					log_close_logfile(log->logfile);
					log->logfile = NULL;
				}
			}

			else
			{

			}
		} /* for() */
	} /* else */
}

int log_write(enum LogType type, unsigned int flags, const char *fmt, ...)
{
	va_list vl;

	va_start(vl, fmt);
	log_writev(type, flags, fmt, vl);
	va_end(vl);
	return 0;
}

int log_writev(enum LogType type, unsigned int flags, const char *fmt, va_list vl)
{
	char buf[LOGMAXLEN+1];
	struct LogSys *log = logsys_bytype(type);
	unsigned int todo = 0;
	size_t len = 0;

	flags |= log->flag;

	/* Extract log-actions to perform */

	if(flags & LOG_DOLOG && !(flags & LOG_DONTLOG)
		&& log->logfile && log->logfile->file)
			todo |= LOG_DOLOG;

	if(flags & LOG_DOWALLOPS && bot.sock >= 0)
		todo |= LOG_DOWALLOPS;

	if(flags & LOG_DOTTY && isatty(1))
		todo |= LOG_DOTTY;

	/* Nothing to do, save CPU, exit */
	if(!todo) return 0;

	/* Format buffer. */
	len = myvsnprintf(buf, LOGMAXLEN, fmt, vl);

	if(todo & LOG_DOLOG)
	{
		/* Write date+time */
		fputs(get_time(NULL, CurrentTS), log->logfile->file);
		fputc(' ', log->logfile->file);

		/* Then LogEventType name */
		fputs(log->name, log->logfile->file);
		fputc(' ', log->logfile->file);

		/* Finally, append buffer and newline */
		fputs(buf, log->logfile->file);
		fputc('\n', log->logfile->file);

		/* Flush to disk unless we're told not to do so */
		if(!(flags & LOG_DONTFLUSH)) fflush(log->logfile->file);
	}

	if(todo & LOG_DOWALLOPS) cswallops("[%s] %s", log->name, buf);

	if(todo & LOG_DOTTY) puts(buf);

	return 0;
}

int log_need_action(enum LogType type)
{
	struct LogSys *log = logsys_bytype(type);

	if((log->flag & LOG_DOLOG && !(log->flag & LOG_DONTLOG) /* write */
		&& log->logfile && log->logfile->file)
		|| (log->flag & LOG_DOWALLOPS && bot.sock >= 0) /* wallops */
		|| (log->flag & LOG_DOTTY && isatty(1))) /* print to console */
		return 1;

	return 0;
}

int log_fd_in_use(int fd)
{
	int i = 0;

	for(; i < LOG_TYPE_COUNT; ++i)
	{
		struct LogSys *log = &LogSystems[i];

		if(!log->logfile) continue;

		if(fileno(log->logfile->file) == fd) return 1;
	}

	return 0;
}
