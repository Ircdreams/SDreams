/* src/lang.c - Gestion du multilangage
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
 * $Id: lang.c,v 1.16 2007/12/01 02:22:31 romexzf Exp $
 */

#include "main.h"
#include "outils.h"
#include "debug.h"
#include "mylog.h"

int LangCount = 0;

Lang *lang_isloaded(const char *name)
{
	Lang *lang = DefaultLang;
	for(; lang && strcasecmp(lang->langue, name); lang = lang->next);
	return lang;
}

int lang_add(char *name)
{
	Lang *lang = lang_isloaded(name);
	FILE *f;
	char path[501];
	int items = 0, i = 0;

	strlwr(name);

	if(!lang) /* This language was not loaded yet */
	{
		Lang *temp = DefaultLang;

		if(!(lang = calloc(1, sizeof *lang)))
		{
			Debug(W_MAX, "lang_add, malloc a échoué pour Lang %s", name);
			return -1;
		}

		Strncpy(lang->langue, name, LANGLEN);
		lang->next = NULL;
		lang->id = LangCount++;

		if(!DefaultLang) DefaultLang = lang;
		else
		{
			for(; temp->next; temp = temp->next); /* append it to the list */
			if(temp) temp->next = lang;
		}
	}

	mysnprintf(path, sizeof path, LANG_PATH "/%s.lang", name);
	if(!(f = fopen(path, "r")))
	{
		log_write(LOG_MAIN, LOG_DOTTY|LOG_DOWALLOPS,
			"lang: fichier %s.lang non trouvé à '%s'", name, path);
		return -1;
	}

	while(fgets(path, sizeof path, f))
	{
		char *msg = strchr(path, ' '); /* split "id message" */
		int msgid = 0, size;

		if(*path == '#' || !msg) continue;
		else *msg++ = 0;

		if(!Strtoint(path, &msgid, 0, LANGMSGNB-1)) /* < 0 || >= LANGMSGNB */
		{
			log_write(LOG_MAIN, LOG_DOTTY|LOG_DOWALLOPS,
				"lang: ID de msg %s inconnu lors du chargement du langage %s",
				path, name);
			continue;
		}
		strip_newline(msg);
		++items; /* increment here otherwise it'll spam upon rehash */

		/* no change since last load */
		if(lang->msg[msgid] && !strcmp(msg, lang->msg[msgid])) continue;

		size = strlen(msg);
		if(size > LANGMSGMAX)
		{
			log_write(LOG_MAIN, LOG_DOTTY|LOG_DOWALLOPS,
				"lang: langage %s: msg %d trop long (%d) a été tronqué,"
				"cela peut causer des erreurs au runtime", name, msgid, size);
			size = LANGMSGMAX;
		}

		if(lang != DefaultLang && lang->msg[msgid] == DefaultLang->msg[msgid])
			lang->msg[msgid] = NULL; /* it's an alias to the default one if was missing. */

		if(!(lang->msg[msgid] = realloc(lang->msg[msgid], size + 1)))
		{
			Debug(W_MAX, "lang_add, malloc a échoué pour Lang %s id %d", name, msgid);
			fclose(f);
			return -1;
		}
		Strncpy(lang->msg[msgid], msg, size);
	}

	if(items < LANGMSGNB) /* hum incomplete language set.. */
	{
		log_write(LOG_MAIN, LOG_DOTTY|LOG_DOWALLOPS,
			"lang: langage %s est incomplet (%d/%d messages)", name, items, LANGMSGNB);

		for(; i < LANGMSGNB; ++i)
			/* .. alias all missing replies to default language. */
			if(!lang->msg[i]) lang->msg[i] = DefaultLang->msg[i];
	}

	fclose(f);
	return 1;
}

int lang_clean(void)
{
	Lang *lang = DefaultLang, *lang_t;
	int i;

	/* remove all alias to Default Language to avoid multiple free() */
	if(DefaultLang->next)
		for(lang = DefaultLang->next; lang; lang = lang->next)
			for(i = 0; i < LANGMSGNB; ++i)
				if(lang->msg[i] == DefaultLang->msg[i]) lang->msg[i] = NULL;

	/* now free all rows */
	for(lang = DefaultLang; lang; lang = lang_t)
	{
		lang_t = lang->next;
		for(i = 0; i < LANGMSGNB; ++i) free(lang->msg[i]);
		free(lang);
	}

	return 0;
}

int lang_check_default(void)
{
	int i = 0;

	if(!DefaultLang) return 0;

	for(; i < LANGMSGNB; ++i)
		if(!DefaultLang->msg[i])
		{
			log_write(LOG_MAIN, LOG_DOTTY, "lang: langage par défaut incomplet"
				" (msgid %d manquante)", i);
			return 0;
		}

	return 1;
}
