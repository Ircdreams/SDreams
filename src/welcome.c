/* src/welcome.c - Diverses commandes sur le module welcome
 *
 * Copyright (C) 2002-2006 David Cortier  <Cesar@ircube.org>
 *                         Romain Bignon  <Progs@ir3.org>
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
 * $Id: welcome.c,v 1.30 2006/12/06 23:02:48 romexzf Exp $
 */

#include "main.h"
#ifdef USE_WELCOMESERV
#include "cs_cmds.h"
#include "debug.h"
#include "mylog.h"
#include "config.h"
#include "outils.h"
#include "welcome.h"

static struct welcomeinfo *welcomehead = NULL;
static int WelcomeCount = 0;

static struct welcomeinfo *gwelcome_add(const char *msg)
{
	struct welcomeinfo *w = malloc(sizeof *w);

   	if(!w)
   	{
	   	Debug(W_MAX, "add_loadwelcome, malloc a échoué pour le welcomeinfo %s", msg);
	   	return NULL;
   	}

	Strncpy(w->msg, msg, sizeof w->msg - 1);
	w->id = ++WelcomeCount;
	w->view = 0;
	w->next = welcomehead;
	welcomehead = w;
	return w;
}

/* GLOBWELCOME ADD <level> <message>
 *             DEL <id>
 *             SET [ON|OFF]
 *             LIST
 *             MOD <id> <level>
 */

int global_welcome(aNick *nick, aChan *chan, int parc, char **parv)
{
	const char *cmd = parv[1];

	if(!strcasecmp(cmd, "SET"))
		switch_option(nick, parc > 1 ? parv[2] : NULL, "globwelcome", "serveur",
			&ConfFlag, CF_WELCOME);

	else if(!strcasecmp(cmd, "ADD"))
	{
		struct welcomeinfo *w;

		if(parc < 2) return csreply(nick, "Syntaxe: %s ADD <message>", parv[0]);

		if((w = gwelcome_add(parv2msg(parc, parv, 2, 250))))
			csreply(nick, "\2[Welcome #%d]\2 - %s", w->id, w->msg);
	}
	else if(!strcasecmp(cmd, "DEL"))
	{
		struct welcomeinfo **tmpp = &welcomehead, *tmp;
		int id;

		if(parc < 2) return csreply(nick, "Syntaxe: %s DEL <ID>", parv[0]);

		if(!Strtoint(parv[2], &id, 1, WelcomeCount))
			return csreply(nick, "Veuillez donner un ID correct");

		for(; (tmp = *tmpp); tmpp = &tmp->next)
			if(id == tmp->id)
			{
				*tmpp = tmp->next;
				free(tmp);
				for(--WelcomeCount, tmp = welcomehead; tmp; tmp = tmp->next)
					if(tmp->id >= id) --tmp->id;
				id = -1; /* mark it as found */
				break;
			}

		if(id >= 0)	return csreply(nick, "Aucun message ayant pour id %d", id);

		csreply(nick, "Le message #%s a bien été supprimé (%d messages restants)",
			parv[2], WelcomeCount);
	}
	else if(!strcasecmp(cmd, "LIST"))
	{
		struct welcomeinfo *w = welcomehead;

		if(!w) return csreply(nick, "Aucun message de bienvenue.");

		csreply(nick, "\2#Id View Message");
		for(; w; w = w->next) csreply(nick, "#%d %d %s", w->id, w->view, w->msg);

		return 0;
	}
	else return csreply(nick, GetReply(nick, L_UNKNOWNOPTION), cmd);

	write_welcome();
	return 0;
}

void choose_welcome(const char *num)
{
	static struct welcomeinfo *tmp;

	if(!welcomehead) return; /* no welcome */
	if(!tmp) tmp = welcomehead;

	++tmp->view;
	putserv("%s %s %s :\2[Welcome #%d]\2 - %s", cs.num, TOKEN_NOTICE, num, tmp->id, tmp->msg);

	tmp = tmp->next;
}

void write_welcome(void)
{
	FILE *fp = fopen(WELCOME_FILE, "w");
	struct welcomeinfo *w = welcomehead;

	if(!fp)
	{
		log_write(LOG_DB, 0, "welcome::write: fopen() failed: %s", strerror(errno));
		return;
	}

	fprintf(fp, "%d\n%s\n%s\n", GetConf(CF_WELCOME), user_motd, admin_motd);
	for(;w;w = w->next) fprintf(fp, "%d :%s\n", w->id, w->msg);
   	fclose(fp);
}

void load_welcome(void)
{
	char buf[420], *tmp;
	int t = 1;
	FILE *fp = fopen(WELCOME_FILE, "r");

    if(!fp)
	{
		log_write(LOG_DB, LOG_DOTTY, "welcome::load: fopen() failed: %s", strerror(errno));
		return;
	}

	while(fgets(buf, sizeof buf, fp))
   	{
		strip_newline(buf);
		if(!strncmp(buf, "VERSION", 7)) continue;
		else if(t == 1)
		{
			if(strtol(buf, NULL, 10)) ConfFlag |= CF_WELCOME;
		}
		else if(t == 2) Strncpy(user_motd, buf, 390);
		else if(t == 3) Strncpy(admin_motd, buf, 390);
		else if((tmp = strchr(buf, ':'))) gwelcome_add(++tmp);
		t++;
	}
	fclose(fp);
}

#endif
