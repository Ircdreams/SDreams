/* src/chanopt.c - Paramétrage des options salons
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
 * $Id: chanopt.c,v 1.55 2007/11/17 16:57:16 romexzf Exp $
 */

#include "main.h"
#include "outils.h"
#include "cs_cmds.h"
#include "ban.h"
#include "hash.h"

/*
 * generic_chanopt # OPT Arg..
 */

int generic_chanopt(aNick *nick, aChan *chan, int parc, char **parv)
{
	const char *c = parv[1], *cmd = parv[2];
	int i = 0;

	if(!strcasecmp(cmd, "defmodes"))
	{	/* nouveau parsage, plus couteux, mais plus fiable -- verif dès le début -Cesar */
		struct cmode tmp = {0, 0, {0}};
		char *key, *limit;
		int k = 0, l = 0;

		if(parc < 3)
			return csntc(nick, "Syntaxe: %s #salon DEFMODES <modes>", RealCmd("chanopt"));

		if((key = strchr(parv[3], 'k'))) k = 4; /* +[k!][l?] */
		if((limit = strchr(parv[3], 'l')) && key > limit) l = k++; /* +[l!][k!] */
		else if(limit) l = key ? 5 : 4; /* +[k!|?][l!] */

		if(!string2scmode(&tmp, parv[3], (k && k <= parc) ? parv[k] : NULL,
			(l && l <= parc) ? parv[l] : NULL))
				return csntc(nick, "Aucun mode valide défini.");
		if(!IsOper(nick) && tmp.modes & C_MOPERONLY) tmp.modes &= ~C_MOPERONLY;

		chan->defmodes = tmp;
		if(CJoined(chan)) modes_reset_default(chan);

		csntc(nick, GetReply(nick, L_DEFMODESARE), c, GetCModes(chan->defmodes));
	}

	else if(!strcasecmp(cmd, "welcome"))
	{
		if(parc < 3)
			return csntc(nick, "Syntaxe: %s #salon WELCOME <message|'none'>", RealCmd("chanopt"));

		if(!strcasecmp(parv[3], "none"))
		{
			*chan->welcome = '\0';
			DelCSetWelcome(chan);
			csntc(nick, GetReply(nick, L_OKDELETED), c);
		}
		else
		{
			parv2msgn(parc, parv, 3, chan->welcome, TOPICLEN);
			csntc(nick, GetReply(nick, L_WELCOMEIS), c, chan->welcome);
		}
	}

	else if(!strcasecmp(cmd, "deftopic"))
	{
		if(parc < 3)
			return csntc(nick, "Syntaxe: %s #salon DEFTOPIC <topic>", RealCmd("chanopt"));

		if(CLockTopic(chan)) return csntc(nick, GetReply(nick, L_LOCKTOPIC), c);

		parv2msgn(parc, parv, 3, chan->deftopic, TOPICLEN);
		csntc(nick, GetReply(nick, L_DEFTOPICIS), c, chan->deftopic);
		cstopic(chan, chan->deftopic);
	}

	else if(!strcasecmp(cmd, "motd"))
	{
		if(parc < 3) return csntc(nick, "Syntaxe: %s #salon MOTD <motd>", RealCmd("chanopt"));

		if(!strcasecmp(parv[3], "none"))
		{
			if(chan->motd) free(chan->motd), chan->motd = NULL;
			csntc(nick, GetReply(nick, L_OKDELETED), c);
		}
		else
		{
			str_dup(&chan->motd, parv2msg(parc, parv, 3, TOPICLEN));
			csntc(nick, GetReply(nick, L_MOTDIS), c, chan->motd);
		}
	}

	else if(!strcasecmp(cmd, "theme"))
	{
		if(parc < 3)
			return csntc(nick, "Syntaxe: %s #salon theme <thème>", RealCmd("chanopt"));

		parv2msgn(parc, parv, 3, chan->description, DESCRIPTIONLEN);
		csntc(nick, GetReply(nick, L_DESCRIPTIONIS), c, chan->description);
	}

	else if(!strcasecmp(cmd, "chanurl"))
	{
		if(parc < 3)
			return csntc(nick, "Syntaxe: %s #salon CHANURL <url>", RealCmd("chanopt"));

		Strncpy(chan->url, parv[3], sizeof chan->url -1);
		csntc(nick, GetReply(nick, L_CHANURLIS), c, chan->url);
	}

	else if(!strcasecmp(cmd, "banlevel"))
	{
		int l = ChanLevelbyUserI(nick->user, chan);

		if(parc < 3)
			return csntc(nick, "Syntaxe: %s #salon BANLEVEL <niveau>", RealCmd("chanopt"));

		if(!is_num(parv[3])) return csntc(nick, GetReply(nick, L_VALIDLEVEL));

		if((i = strtol(parv[3], NULL, 10)) > l && !IsAdmin(nick->user))
			return csntc(nick, GetReply(nick, L_MAXLEVELISYOURS));

		if(chan->banlevel > l && !IsAdmin(nick->user))
			return csntc(nick, GetReply(nick, L_ACCESSTOOLOW), c, l, chan->banlevel);

		chan->banlevel = i;
		csntc(nick, GetReply(nick, L_BANLEVELIS), c, chan->banlevel);
	}

	else if(!strcasecmp(cmd, "bantype"))
	{
		if(parc < 3)
			return csntc(nick, "Syntaxe: %s #salon BANTYPE <n° du type>", RealCmd("chanopt"));

		if(!Strtoint(parv[3], &i, 1, 5))
			return csntc(nick, "Le type de ban est un nombre entre 1 et 5.");

		chan->bantype = i;
		csntc(nick, GetReply(nick, L_BANTYPEIS), chan->chan, GetBanType(chan), i);
	}

	else if(!strcasecmp(cmd, "strictop"))
		switch_option(nick, parc < 3 ? NULL : parv[3], cmd, c, &chan->flag, C_STRICTOP);

	else if(!strcasecmp(cmd, "locktopic"))
		switch_option(nick, parc < 3 ? NULL : parv[3], cmd, c, &chan->flag, C_LOCKTOPIC);

	else if(!strcasecmp(cmd, "nobans"))
		switch_option(nick, parc < 3 ? NULL : parv[3], cmd, c, &chan->flag, C_NOBANS);

	else if(!strcasecmp(cmd, "noops"))
		switch_option(nick, parc < 3 ? NULL : parv[3], cmd, c, &chan->flag, C_NOOPS);

	else if(!strcasecmp(cmd, "novoices"))
		switch_option(nick, parc < 3 ? NULL : parv[3], cmd, c, &chan->flag, C_NOVOICES);

	else if(!strcasecmp(cmd, "autoinvite"))
		switch_option(nick, parc < 3 ? NULL : parv[3], cmd, c, &chan->flag, C_AUTOINVITE);

	else if(!strcasecmp(cmd, "setwelcome"))
		switch_option(nick, parc < 3 ? NULL : parv[3], cmd, c, &chan->flag, C_SETWELCOME);

	else if(!strcasecmp(cmd, "noinfo"))
		switch_option(nick, parc < 3 ? NULL : parv[3], cmd, c, &chan->flag, C_NOINFO);

	else if(!strcasecmp(cmd, "chmodeslevel"))
	{
		int l = ChanLevelbyUserI(nick->user, chan);
		if(parc < 3)
			return csntc(nick, "Syntaxe: %s #salon CHMODESLEVEL <niveau>", RealCmd("chanopt"));

		if(!is_num(parv[3])) return csntc(nick, GetReply(nick, L_VALIDLEVEL));

		if((i = strtol(parv[3], NULL, 10)) > l && !IsAdmin(nick->user))
			return csntc(nick, GetReply(nick, L_MAXLEVELISYOURS));

		if(chan->cml > l && !IsAdmin(nick->user))
			return csntc(nick, GetReply(nick, L_ACCESSTOOLOW), c, l, chan->cml);

		chan->cml = i;
		csntc(nick, GetReply(nick, L_CHMODESLEVELIS), c, chan->cml);
	}

	else if(!strcasecmp(cmd, "bantime"))
	{
		time_t timeout = 0;
		if(parc < 3)
			return csntc(nick, "Syntaxe: %s #salon BANTIME <durée>", RealCmd("chanopt"));

		if((timeout = convert_duration(*parv[3] == '%' ? ++parv[3] : parv[3])) < 0)
			return csntc(nick, "Durée incorrecte, le format est <XjXhXm> "
						"X étant le nombre respectif d'unités, ou 0 pour désactiver.");

		chan->bantime = timeout;
		if(timeout) csntc(nick, GetReply(nick, L_BANTIMEIS), c, chan->bantime, duration(timeout));
		else csntc(nick, GetReply(nick, L_OPTIONNOWOFF), cmd, c);
	}

	else if(!strcasecmp(cmd, "autolimit"))
	{
		if(parc < 3)
		{
			chan->flag ^= C_FLIMIT;
			if(CFLimit(chan) && (!chan->limit_inc || !chan->limit_min))
			{
				chan->limit_inc = DEFAUT_LIMITINC;
				chan->limit_min = DEFAUT_LIMITMIN;
			}
		}
		else if(!strcasecmp(parv[3], "off")) chan->flag &= ~C_FLIMIT;
		else
		{
			int linc = getoption("-limit_inc", parv, parc, 3, GOPT_INT);
			int lmin = getoption("-limit_min", parv, parc, 3, GOPT_INT);

			if((strcasecmp(parv[3], "on") && !lmin && !linc) || lmin < 0 || linc < 0)
				return csntc(nick, "Syntaxe: %s #salon AUTOLIMIT [ON|OFF]"
							" [-limit_inc <nombre>] [-limit_min <nombre>]", RealCmd("chanopt"));

			chan->limit_inc = linc ? linc : (chan->limit_inc ? chan->limit_inc : DEFAUT_LIMITINC);
			chan->limit_min = lmin ? lmin : (chan->limit_min ? chan->limit_min : DEFAUT_LIMITMIN);
			if(!CFLimit(chan)) chan->flag |= C_FLIMIT;

		}
		if(!CFLimit(chan)) csntc(nick, GetReply(nick, L_OPTIONNOWOFF), cmd, c);
		else csntc(nick, GetReply(nick, L_AUTOLIMITIS), c, chan->limit_min, chan->limit_inc);
		if(CJoined(chan)) floating_limit_update_timer(chan);
	}
	else return csntc(nick, GetReply(nick, L_UNKNOWNOPTION), cmd);
	return 1;
}

static int do_chanopt(aNick *nick, aChan *chan, int parc, char **parv, char *type)
{
	static char *pparv[50];
	int i = 2, c = parc > ASIZE(pparv) - 2 ? ASIZE(pparv) - 2 : parc;

	pparv[1] = parv[1];
	pparv[2] = type;

	for(;i <= c;++i) pparv[i + 1] = parv[i];

	return generic_chanopt(nick, chan, i, pparv);
}

int defmodes(aNick *nick, aChan *chan, int parc, char **parv)
{
	return do_chanopt(nick, chan, parc, parv, "defmodes");
}

int description(aNick *nick, aChan *chan, int parc, char **parv)
{
	return do_chanopt(nick, chan, parc, parv, "theme");
}

int csetwelcome(aNick *nick, aChan *chan, int parc, char **parv)
{
	return do_chanopt(nick, chan, parc, parv, "welcome");
}

int define_motd(aNick *nick, aChan *chan, int parc, char **parv)
{
	return do_chanopt(nick, chan, parc, parv, "motd");
}

int deftopic(aNick *nick, aChan *chan, int parc, char **parv)
{
	return do_chanopt(nick, chan, parc, parv, "deftopic");
}

int locktopic(aNick *nick, aChan *chan, int parc, char **parv)
{
	char *pparv[5] = { "chanopt", parv[1], "locktopic", parv[2], NULL};

	return generic_chanopt(nick, chan, parc + 1, pparv);
}

int strictop(aNick *nick, aChan *chan, int parc, char **parv)
{
	char *pparv[5] = { "chanopt", parv[1], "strictop", parv[2], NULL};

	return generic_chanopt(nick, chan, parc + 1, pparv);
}

int nobans(aNick *nick, aChan *chan, int parc, char **parv)
{
	char *pparv[5] = { "chanopt", parv[1], "nobans", parv[2], NULL};

	return generic_chanopt(nick, chan, parc + 1, pparv);
}

int noops(aNick *nick, aChan *chan, int parc, char **parv)
{
	char *pparv[5] = { "chanopt", parv[1], "noops", parv[2], NULL};

	return generic_chanopt(nick, chan, parc + 1, pparv);
}

int activwelcome(aNick *nick, aChan *chan, int parc, char **parv)
{
	char *pparv[5] = { "chanopt", parv[1], "setwelcome", parv[2], NULL};

	return generic_chanopt(nick, chan, parc + 1, pparv);
}

int banlevel(aNick *nick, aChan *chan, int parc, char **parv)
{
	char *pparv[5] = { "chanopt", parv[1], "banlevel", parv[2], NULL};

	return generic_chanopt(nick, chan, parc + 1, pparv);
}

int bantype(aNick *nick, aChan *chan, int parc, char **parv)
{
	char *pparv[5] = { "chanopt", parv[1], "bantype", parv[2], NULL};

	return generic_chanopt(nick, chan, parc + 1, pparv);
}

int chanurl(aNick *nick, aChan *chan, int parc, char **parv)
{
	char *pparv[5] = { "chanopt", parv[1], "chanurl", parv[2], NULL};

	return generic_chanopt(nick, chan, parc + 1, pparv);
}
