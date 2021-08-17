/* src/aide.c - Aide
 *
 * Copyright (C) 2002-2006 David Cortier  <Cesar@ircube.org>
 *                         Romain Bignon  <Progs@ir3.org>
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
 * $Id: aide.c,v 1.37 2006/12/06 23:02:44 romexzf Exp $
 */

#include "main.h"
#include "cs_cmds.h"
#include "outils.h"
#include "debug.h"
#include "mylog.h"
#include "hash.h"
#include "showcommands.h"

int syntax_cmd(aNick *nick, aHashCmd *cmd)
{
	static char helpcmd[CMDLEN + 1] = {0};

	if(!*cmd->syntax) return csreply(nick, "\2Erreur de syntaxe !");
	if(helpcmd[0] == 0) strcpy(helpcmd, RealCmd("aide"));

	csreply(nick, cmd->syntax, cmd->name);
	csreply(nick, GetReply(nick, L_MOREHELP), cs.nick, helpcmd, cmd->name);
	return 1;
}

int aide(aNick *nick, aChan *chan, int parc, char **parv)
{
	char more[CMDLEN + 1], *list;
	int morelen = 0, cur = 0, i = 0;
	aHashCmd *cmdp;
	HelpBuf *help;

	if(!parc) return showcommands(nick, chan, parc, parv);

	if(!(cmdp = FindCommand(parv[1]))) return csreply(nick, GetReply(nick, L_NOSUCHCMD), parv[1]);

	help = cmdp->help[nick->user ? nick->user->lang->id : DefaultLang->id];

	if(!help || !help->buf)
	{
		log_write(LOG_MAIN, LOG_DOWALLOPS, "No help found for command %s, lang %s",
			parv[1], nick->user ? nick->user->lang->langue : DefaultLang->langue);
		csreply(nick, "\2Aide non trouvée, consultez un Administrateur");
		return 0;
	}

	if(parc > 1)
	{
		Strncpy(more, parv[2], CMDLEN);
		morelen = strlen(more);
		csreply(nick, GetReply(nick, L_HELP1), parv[1], more,
			AdmCmd(cmdp) ? "4ADMIN" : ChanCmd(cmdp) ? "12CHAN" : "3USER", cmdp->level);
	}
	else
	{
		csreply(nick, GetReply(nick, L_HELP2), parv[1],
			AdmCmd(cmdp) ? "4ADMIN" : ChanCmd(cmdp) ? "12CHAN" : "3USER", cmdp->level);
		if(*cmdp->syntax) csreply(nick, cmdp->syntax, cmdp->name);
	}

	list = help->buf[help->count-1];

	for(; i < help->count; ++i)
	{
		char *p = help->buf[i];
		if(*p == '|') /* subsection found */
		{
			if(morelen && !cur) /* was I looking for a subsection ? */
			{
				if(strncasecmp(++p, more, morelen)) /* not this one, go on */
					continue;
				else cur = 1; /* found.. */
			}
			else break; /* found a pipe and was not looking for option or already found */
		}
		else if(morelen && !cur) continue;
		csreply(nick, "%s", p);
	}

	/* looking for option, but not found! */
	if(morelen && !cur)	csreply(nick, "Aucune aide sur \2%s\2 pour la commande %s", more, parv[1]);
	/* there are options but none found/searched */
	if(*list == '|' && !cur) csreply(nick, GetReply(nick, L_OPTIONS), list + 1);
	/* option found or none looked for (search success!) */
	if(cur || !morelen) csreply(nick, GetReply(nick, L_SHOWCMDHELP2), bot.chan);
	if(DisableCmd(cmdp)) csreply(nick, "\2\0034Note:\2\3 %s", GetReply(nick, L_CMDDISABLE));

	return 1;
}

static HelpBuf *help_newbuf(void)
{
	HelpBuf *ptr = calloc(1, sizeof *ptr);
	if(!ptr) Debug(W_MAX, "help::load, newbuf failed!");

	return ptr;
}

static int help_addbuf(HelpBuf *help, const char *buf)
{
	if(!(help->buf = realloc(help->buf, sizeof *help->buf * ++help->count)))
		return Debug(W_MAX, "help::load, addbuf OOM! (%s)", buf);

	help->buf[help->count -1] = NULL;
	str_dup(&help->buf[help->count -1], buf);
	return 0;
}

static int help_read_file(Lang *lang, FILE *fp)
{
	char buf[300], list[200], *ptr = list;
	aHashCmd *cmdp = NULL;
	int cmds = 0, drop = 1;

	while(fgets(buf, sizeof buf, fp))
	{
		strip_newline(buf);

		if(*buf == '#') /* got a new command */
		{	/* first, end with previsous command options list, if any */
			if(cmdp && *list) help_addbuf(cmdp->help[lang->id], list);

			if(!(cmdp = FindCoreCommand(buf + 1)))
			{
				/*Debug(0, "help::loading lang %s, unknown command %s", lang->langue, buf + 1);*/
				drop = 1;
				continue;
			} /* help** has already been realloc'ed. or must be. */
			else drop = 0;

			if(cmdp->help[lang->id]) /* clean up current help */
			{
				int i = 0;
				for(; i < cmdp->help[lang->id]->count; ++i) free(cmdp->help[lang->id]->buf[i]);
				free(cmdp->help[lang->id]->buf);
				cmdp->help[lang->id]->buf = NULL;
				cmdp->help[lang->id]->count = 0;
			}
			else cmdp->help[lang->id] = help_newbuf();
			*list = 0; /* reinit list */
			ptr = list;
			++cmds;
		}
		else if(drop) continue;
		else if(*buf == '!') /* syntax */
		{
			if(lang == DefaultLang) Strncpy(cmdp->syntax, buf + 1, SYNTAXLEN);
		}
		else if(*buf == '|') /* subsection */
		{
			char *p = strchr(buf + 1, ' ');

			if(p) help_addbuf(cmdp->help[lang->id], buf), *p = 0;
			/* append option to subsections' list */
			ptr += mysnprintf(ptr, list + sizeof list - ptr, "%c%s",
					*list ? ' ' : '|', buf + 1);
		}
		else if(*buf) help_addbuf(cmdp->help[lang->id], buf);
	}
	return cmds;
}

int help_load(Lang *lang)
{
	FILE *fp;
	char buf[100];
	int i;

	if(!lang)
	{
		for(lang = DefaultLang; lang; lang = lang->next) help_load(lang);
		return 0;
	}
	mysnprintf(buf, sizeof buf, "aide/%s.help", lang->langue);

	if(!(fp = fopen(buf, "r")))
	{
		log_write(LOG_DB, LOG_DOTTY, "help::load:(%s) fopen() failed: %s",
			lang->langue, strerror(errno));
		return -1; /* error */
	}

	i = help_read_file(lang, fp);
	fclose(fp);

	if(i != CmdsCount)
		log_write(LOG_DB, LOG_DOTTY, "help::load:  Missing %d help sections for lang %s",
			CmdsCount - i, lang->langue);

	return i;
}

/* Realloc Help blocks for each command, note that LangCount CAN NOT be below oldcount */
int help_cmds_realloc(int oldcount)
{
	register aHashCmd *tmp;
	int i = 0, j;

	for(; i < CMDHASHSIZE; ++i) for(tmp = cmd_hash[i]; tmp; tmp = tmp->next)
	{
		void *p = realloc(tmp->help, sizeof *tmp->help * LangCount);
		if(!p) Debug(W_MAX, "help::realloc, OOM!");
		else for(j = oldcount, tmp->help = p; j <= LangCount; ++j) tmp->help[j] = NULL;
	}

	return 0;
}
