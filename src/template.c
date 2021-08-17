/* src/template.c - Template
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
 * $Id: template.c,v 1.9 2007/12/01 21:42:10 romexzf Exp $
 */

#include "main.h"
#include "template.h"
#include "debug.h"
#include "config.h"

#define TMPLBUFSIZE 512

struct Template {
	char **buf;
	int count;
} tmpl_mail_register, tmpl_mail_oubli;

static int tmpl_realloc(struct Template *tpl, size_t size)
{
	void *temp;

	if(!size)
	{
		temp = realloc(tpl->buf, ++tpl->count * sizeof *tpl->buf);
		if(!temp) return Debug(W_MAX, "tmpl::realloc OOM!");
		tpl->buf = temp;
		tpl->buf[tpl->count-1] = NULL;
	} /* if size is != 0, then addstr requires more bytes to add last line */

	temp = realloc(tpl->buf[tpl->count-1], (size ? size : TMPLBUFSIZE) + 1);
	if(!temp) return Debug(W_MAX, "tmpl::realloc OOM!");

	tpl->buf[tpl->count-1] = temp;
	return 1;
}

static size_t tmpl_load_addstr(struct Template *tpl, const char *str,
								size_t used, size_t size)
{
	size_t needed = size + used;
	if(!size) return 0;
	/* need some byte more than allocated to write str */
	if(needed >= TMPLBUFSIZE) tmpl_realloc(tpl, needed);

	strcpy(tpl->buf[tpl->count -1] + used, str);
	return needed >= TMPLBUFSIZE ? 0 : needed; /* 0 means line is full */
}

static void tmpl_load_atmpl(struct Template *tpl, FILE *f)
{
	int i = 0;
	size_t used = 0;
	char buf[512];

	if(tpl->count) /* clean up */
	{
		for(; i < tpl->count; ++i) free(tpl->buf[i]);
		free(tpl->buf), tpl->count = 0, tpl->buf = NULL;
	}

	while(fgets(buf, sizeof buf, f))
	{
		if(!used) tmpl_realloc(tpl, 0); /* new line: allocate BUFSIZE+1 */
		used = tmpl_load_addstr(tpl, buf, used, strlen(buf)); /* addstr realloc if needed */
	}
}

int tmpl_load(void)
{
	FILE *f = fopen(LANG_PATH "/mailregister.tmpl", "r");

	if(!f) return 0;
	tmpl_load_atmpl(&tmpl_mail_register, f);
	fclose(f);

	if(!(f =  fopen(LANG_PATH "/mailoubli.tmpl", "r"))) return 0;
	tmpl_load_atmpl(&tmpl_mail_oubli, f);
	fclose(f);
	return 1;
}

int tmpl_mailsend(struct Template *tpl, const char *mail, const char *user,
				const char *pass, const char *host)
{
	FILE *fm;
	int i = 0;
	size_t size = 0;
	char buf[2 * TMPLBUFSIZE];

	if(!tpl->count) return Debug(W_WARN, "No template loaded for mailing!");
	if(!(fm = popen(cf_mailprog, "w")))
		return Debug(W_WARN, "popen() failed for %s [%s]", mail, strerror(errno));

	for(; i < tpl->count; ++i)
	{
		const char *p = tpl->buf[i];

#define AddStr(s) do { \
	size_t len = strlen(s); \
	if(size + len >= sizeof buf) buf[size] = 0, fputs(buf, fm), size = 0; \
	strcpy(buf + size, s); \
	size += len; \
} while(0)

		while(*p)
			if(*p == '$')
			{
				if(*++p == 'u') AddStr(user);
				else if(*p == 'e') AddStr(mail);
				else if(*p == 'p') AddStr(pass);
				else if(*p == 'h') AddStr(host);
				else --p; /* go back */
				++p;
			}
			else
			{
				if(size >= sizeof buf -1) buf[size] = 0, fputs(buf, fm), size = 0;
				buf[size++] = *p++;
			}
	}
	buf[size] = 0;
	fputs(buf, fm); /* dump line */

	pclose(fm);
	return 1;
}

int tmpl_clean(void)
{
	int i;

	for(i = 0; i < tmpl_mail_register.count; ++i)
		free(tmpl_mail_register.buf[i]);
	free(tmpl_mail_register.buf);

	for(i = 0; i < tmpl_mail_oubli.count; ++i)
		free(tmpl_mail_oubli.buf[i]);
	free(tmpl_mail_oubli.buf);

	return 0;
}
