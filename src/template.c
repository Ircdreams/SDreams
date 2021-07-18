/* src/templace.c - Template
 * Copyright (C) 2004-2006 ircdreams.org
 *
 * contact: bugs@ircdreams.org
 * site web: http://www.ircdreams.org
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
 * $Id: template.c,v 1.13 2006/03/16 07:08:43 bugs Exp $
 */

#include "main.h"
#include "lang.h"
#include "debug.h"
#include "config.h"
#include "template.h"
#include <errno.h>

#define TMPLBUFSIZE 512

struct Template {
	char **buf;
	int count;
} tmpl_mail_register, tmpl_mail_oubli, tmpl_mail_memo;

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

static int tmpl_load_addstr(struct Template *tpl, const char *str, int used, size_t size)
{
	int needed = size + used;
	if(!size) return 0;
	/* need some byte more than allocated to write str */
  	if(needed >= TMPLBUFSIZE) tmpl_realloc(tpl, needed);

	strcpy(tpl->buf[tpl->count -1] + used, str);
	return needed >= TMPLBUFSIZE ? 0 : needed; /* 0 means line is full */
}

static void tmpl_load_atmpl(struct Template *tpl, FILE *f)
{
	int i = 0, used = 0;
	char buf[512];

	if(tpl->count) /* clean up */
	{
		for(;i < tpl->count;++i) free(tpl->buf[i]);
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
	FILE *f = fopen(TMPL_PATH "/mailregister.tmpl", "r");

	if(!f) {
		Debug(W_TTY, "TMPL::load: Erreur lors de la lecture du template mail register: %s", strerror(errno));
		return 0;
	}
	tmpl_load_atmpl(&tmpl_mail_register, f);
	fclose(f);

	if(!(f =  fopen(TMPL_PATH "/mailoubli.tmpl", "r"))) {
		Debug(W_TTY, "TMPL::load: Erreur lors de la lecture du template mail oubli: %s", strerror(errno));
		return 0;
	}
	tmpl_load_atmpl(&tmpl_mail_oubli, f);
	fclose(f);

	if(!(f =  fopen(TMPL_PATH "/mailmemo.tmpl", "r"))) {
		Debug(W_TTY, "TMPL::load: Erreur lors de la lecture du template mail memo: %s", strerror(errno));
		return 0;
	}
	tmpl_load_atmpl(&tmpl_mail_memo, f);
	fclose(f);
	return 1;
}

int tmpl_mailsend(struct Template *tpl, const char *mail, const char *user,
	const char *pass, const char *host, const char *from, const char *texte)
{
	FILE *fm;
	int i = 0, size = 0, sizes = 0;
	char buf[2*TMPLBUFSIZE];

	if(!tpl->count) return Debug(W_WARN, "Aucun template de charger pour l'envoi de mail!");

	if(!(fm = popen(mailprog, "w")))
		return Debug(W_WARN, "popen() failed for %s [%s]", mail, strerror(errno));

	for(;i < tpl->count;++i)
	{
		const char *p = tpl->buf[i];

#define AddStr(s) do {\
	sizes = strlen(s);\
	if(size + sizes >= sizeof buf -1) buf[size] = 0, fputs(buf, fm), size = 0;\
	strcpy(buf + size, s);\
	size += sizes;\
} while(0)

		while(*p)
			if(*p == '$')
			{
				if(*++p == 'u') AddStr(user);
				else if(*p == 'e') AddStr(mail);
				else if(*p == 'p') AddStr(pass);
				else if(*p == 'h') AddStr(host);
				else if(*p == 'f') AddStr(from);
				else if(*p == 't') AddStr(texte);
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

	for(i = 0; i < tmpl_mail_memo.count; ++i)
                free(tmpl_mail_memo.buf[i]);
        free(tmpl_mail_memo.buf);

  	return 0;
}
