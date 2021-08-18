/* src/admin_cmds.c - Diverses commandes pour admins
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
 * $Id: admin_cmds.c,v 1.99 2007/01/02 19:44:30 romexzf Exp $
 */

#include "main.h"
#include "outils.h"
#include "cs_cmds.h"
#include "fichiers.h"
#include "add_info.h"
#include "aide.h"
#include "showcommands.h"
#include "hash.h"
#include "config.h"
#include "template.h"
#include "timers.h"
#ifdef USE_WELCOMESERV
#include "welcome.h"
#endif

int inviteme(aNick *nick, aChan *chan, int parc, char **parv)
{ 	/* invite des admins sur le salon d'infos */
    putserv("%s " TOKEN_INVITE " %s :%s", cs.num, nick->nick, bot.pchan);
    return 1;
}

int die(aNick *nick, aChan *chan, int parc, char **parv)
{
	const char *r = parc ? parv2msg(parc, parv, 1, RAISONLEN) : cf_quit_msg;

	running = 0;

	putserv("%s " TOKEN_QUIT " :%s", cs.num, r);
	putserv("%s " TOKEN_SQUIT " %s 0 :%s", bot.servnum, bot.server, r);
	return 1;
}

/*
 * restart_bot parv[1->parc-1] = reason
 */
int restart_bot(aNick *nick, aChan *chan, int parc, char **parv)
{
	FILE *fuptime;
	const char *r = parc ? parv2msg(parc, parv, 1, RAISONLEN) : cf_quit_msg;

	if((fuptime = fopen("uptime.tmp", "w"))) /* permet de maintenir l'uptime */
	{
		fprintf(fuptime, "%lu", bot.uptime);
		fclose(fuptime);
	}

	running = 0;
	ConfFlag |= CF_RESTART;

	putserv("%s " TOKEN_QUIT " :%s [\2Restarting\2]", cs.num, r);
	putserv("%s "TOKEN_SQUIT" %s 0 :%s [\2RESTART\2]", bot.servnum, bot.server, r);
	return 1;
}

int chcomname(aNick *nick, aChan *chan, int parc, char **parv)
{
	aHashCmd *cmd;
	const char *newcmd = parv[2], *lastcmd = parv[1];

	if(strlen(newcmd) > CMDLEN)
		return csreply(nick, "Un nom de commande ne doit pas dépasser %d caractères.", CMDLEN);

	if(!(cmd = FindCommand(lastcmd)) || IsCTCP(cmd))
		return csreply(nick, GetReply(nick, L_NOSUCHCMD), lastcmd);

	if(FindCommand(newcmd)) return csreply(nick, "La commande %s existe déjà.", newcmd);

	HashCmd_switch(cmd, newcmd);
	csreply(nick, "La commande\2 %s\2 s'appelle maintenant\2 %s\2.", lastcmd, cmd->name);
	BuildCommandsTable(1);
	write_cmds();
	return 1;
}

int chlevel(aNick *nick, aChan *chan, int parc, char **parv)
{
	const char *cmd = parv[1];
	int level, adm = getoption("-admin", parv, parc, 2, GOPT_FLAG);
	aHashCmd *cmdp;

	if(!(cmdp = FindCommand(cmd))) return csreply(nick, GetReply(nick, L_NOSUCHCMD), cmd);

	if(!Strtoint(parv[2], &level, 0, OWNERLEVEL))
		return csreply(nick, GetReply(nick, L_VALIDLEVEL));

	if(level == cmdp->level && !adm)
		return csreply(nick, "%s est déjà au niveau %d.", cmd, level);

	if(!NeedNoAuthCmd(cmdp) && !level)
		return csreply(nick, "La commande %s ne peut être utilisée que par des users"
			" identifiés sur les services. Le niveau minimal est donc de 1.", cmd);

	if(!ChanCmd(cmdp))
	{
		if(level > MAXADMLVL)
			return csreply(nick, "Le niveau maximum Administrateur est %d.", MAXADMLVL);
		else if(level > nick->user->level)
			return csreply(nick, GetReply(nick, L_MAXLEVELISYOURS));
		else
		{ /* détermination du niveau admin ou pas car level 0-4 */
			if(level >= ADMINLEVEL) cmdp->flag |= CMD_ADMIN;
			else cmdp->flag &= ~CMD_ADMIN;
		}
	}
	if(ChanCmd(cmdp) && adm)
	{	 /* argument de switch admin pour les cmds chans (0-500 <= donc nécessaire) */
		if(AdmCmd(cmdp)) cmdp->flag &= ~CMD_ADMIN;
		else cmdp->flag |= CMD_ADMIN;
	}

	cmdp->level = level;

	csreply(nick, "Le niveau de la commande %s est maintenant\2 %d\2 (%s).",
		cmdp->name, cmdp->level, AdmCmd(cmdp) ? "Admin" : "User");
	write_cmds();
	BuildCommandsTable(1);
	return 1;
}

int disable_cmd(aNick *nick, aChan *chan, int parc, char **parv)
{
	const char *cmd = parv[1];
	aHashCmd *cmdp = FindCommand(cmd);

	if(!cmdp) return csreply(nick, GetReply(nick, L_NOSUCHCMD), cmd);

	if(!strcasecmp(cmdp->corename, "DISABLE"))
		return csreply(nick, "Veuillez ne pas desactiver \2%s\2.", cmd);

	switch_option(nick, parv[2], "disable", cmd, &cmdp->flag, CMD_DISABLE);

	BuildCommandsTable(1);
	write_cmds();
	return 1;
}

#ifdef USE_WELCOMESERV
int set_motds(aNick *nick, aChan *c, int parc, char **parv)
{
	char *tmp;
	if(!strcasecmp(parv[1], "LIST"))
	{
		csreply(nick, "Motd User : \2%s\2", user_motd);
		csreply(nick, "Motd Admin: \2%s\2", admin_motd);
		return 1;
	}
	else if(parc < 2) return syntax_cmd(nick, FindCoreCommand("adminmotd"));
	else if(!strcasecmp(parv[1], "USER")) tmp = user_motd;
	else if(!strcasecmp(parv[1], "ADMIN")) tmp = admin_motd;
	else return csreply(nick, "Il n'y a pas de motd pour %s", parv[1]);

	if(strcasecmp("none", parv[2]))
	{
		parv2msgn(parc, parv, 2, tmp, 390);
		csreply(nick, "Le motd pour les %ss est maintenant:\2 %s", parv[1], tmp);
	}
	else *tmp = 0, csreply(nick, "Le motd pour les %ss a bien été supprimé", parv[1]);

	write_welcome();
	return 1;
}
#endif

/* cmds (c) keeo (le 3 en 1 par progs) */
int globals_cmds(aNick *nick, aChan *chan, int parc, char **parv)
{
	const char *cmd = parv[1];
	int i = 0;

	if(!strcasecmp(cmd, "MSG"))
	{
		const char *msg = parv2msg(parc, parv, 2, MEMOLEN);

		for(; i < CHANHASHSIZE; ++i) for(chan = chan_tab[i]; chan; chan = chan->next)
			if(CJoined(chan) && chan->netchan->users > 1)
				putserv("%s "TOKEN_PRIVMSG" %s :\2%s>\2 %s", cs.num, chan->chan, nick->nick, msg);
	}

#ifdef USE_MEMOSERV
	else if(!strcasecmp(cmd, "MEMO"))
	{
		char memo[MEMOLEN + 1];
		anUser *user;
		int min_level = getoption("-min", parv, parc, 2, GOPT_INT);
		int max_level = getoption("-max", parv, parc, 2, GOPT_INT);
		int mask = getoption("-user", parv, parc, 2, GOPT_STR);
		int owner = getoption("-owner", parv, parc, 2, GOPT_FLAG), count = 0;

		for(i = 2; i < parc && *parv[i] == '-'; i += 2);

		if(owner) --i; /* -owner does not expect an argument */

		parv2msgn(parc, parv, i, memo, MEMOLEN);

		if(max_level > MAXADMLVL || min_level > MAXADMLVL)
			return csreply(nick, GetReply(nick, L_VALIDLEVEL));

		for(i = 0; i < USERHASHSIZE; ++i) for(user = user_tab[i]; user; user = user->next)
			if(user != nick->user && (!min_level || user->level >= min_level)
			   && (!max_level || user->level <= max_level)
			   && (!mask || !match(parv[mask], user->nick))
			   && (!owner || IsAnOwner(user)))
			{
				if(user->n && !IsAway(user->n))
					csreply(user->n, "\2MEMO Global de %s :\2 %s", nick->user->nick, memo);
				else add_memo(user, nick->user->nick, CurrentTS, memo, MEMO_AUTOEXPIRE);
				++count;
			}

		csreply(nick, "Le memo suivant a été envoyé à %d users: %s", count, memo);
	}

	else if(!strcasecmp(cmd, "CMEMO")) /* mémo aux accès d'un salon */
	{
		aLink *l;
		char memo[MEMOLEN + 1];
		int level = getoption("-level", parv, parc, 2, GOPT_INT);

		i = level ? 5 : 3; /* message starts at 5 if a level was given */

		if(parc < i)
			return csreply(nick, "Syntaxe: %s CMEMO #channel [-level level] <msg>", parv[0]);

		if(!(chan = getchaninfo(parv[2])))
			return csreply(nick, GetReply(nick, L_NOSUCHCHAN), parv[2]);

		parv2msgn(parc, parv, i, memo, MEMOLEN);

		for(i = 0, l = chan->access; l; l = l->next)
		{
			anAccess *a = l->value.a;
			if(!level || a->level >= level)
			{
				if(a->user->n && !IsAway(a->user->n))
					csreply(a->user->n, "\2MEMO Global de %s :\2 %s", nick->user->nick, memo);
				else add_memo(a->user, nick->user->nick, CurrentTS, memo, MEMO_AUTOEXPIRE);
				++i;
			}
		}

		csreply(nick, "Le memo suivant a été envoyé à %d users: %s", i, memo);
	}
#endif
	else return csreply(nick, GetReply(nick, L_UNKNOWNOPTION), cmd);
	return 1;
}

int rehash_conf(aNick *nick, aChan *chan, int parc, char **parv)
{
	int langs;
	Lang *l;

	if(getoption("-purge", parv, parc, 1, GOPT_FLAG))
	{
		check_accounts();
		check_chans();
		return csreply(nick, "Purge effectuée.");
	}
	if(getoption("-template", parv, parc, 1, GOPT_FLAG))
	{
		tmpl_load();
		return csreply(nick, "Templates reloaded from disk.");
	}
	if((langs = getoption("-aide", parv, parc, 1, GOPT_STR)))
	{
		if((l = lang_isloaded(parv[langs])))
		{
			help_load(l);
			csreply(nick, "Help should have been reload for language %s.", l->langue);
		}
		else csreply(nick, GetReply(nick, L_NOSUCHLANG), parv[langs]);
		return 0;
	}
	langs = LangCount; /* save old count in case of load */

	if(load_config(FICHIER_CONF) == -1)
		return csreply(nick, "Une erreur s'est produite lors de l'actualisation...");

	if(LangCount != langs) /* NB: lang can only be LOADED while rehashing */
	{
		help_cmds_realloc(langs); /* so LangCount >= langs */
		for(l = DefaultLang; l; l = l->next) if(l->id > langs) help_load(l);
		csreply(nick, "%d langs and help loaded.", LangCount - langs);
	}

	return csreply(nick, "Configuration actualisée avec succès");
}

int showconfig(aNick *nick, aChan *c, int parc, char **parv)
{
	csreply(nick, "Etat des variables de configuration");
	csreply(nick, "Uplink: %s/%s:%d", bot.ip, bot.bindip, bot.port);
	csreply(nick, "Nom du serveur des Services : %s (%s) [Num: %s]",
		bot.server, bot.name, bot.servnum);
	csreply(nick, "Salon de controle : %s Salon d'aide : %s", bot.pchan, bot.chan);
	csreply(nick, "Message de quit : %s", cf_quit_msg);
	csreply(nick, "Temps avant un kill : %s ", duration(cf_kill_interval));
	csreply(nick, "Max LastSeen : %s", duration(cf_maxlastseen));
	csreply(nick, "Admin exempté de flood : %s Kill pour flood : %s",
		GetConf(CF_ADMINEXEMPT) ? "Oui" : "Non", GetConf(CF_KILLFORFLOOD) ? "Oui" : "Non");
	csreply(nick, "Temps d'ignore : %s", duration(cf_ignoretime));
	csreply(nick, "Programme de mail : %s", cf_mailprog ? cf_mailprog : "aucun");
	csreply(nick, "Host hidding: %s (Suffixe: %s)",
		GetConf(CF_HOSTHIDING) ? "Oui" : "Non", cf_hidden_host);
#ifdef WEB2CS
	csreply(nick, "Web2CS/Port : %d", bot.w2c_port);
#endif
	return 1;
}
