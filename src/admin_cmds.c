/* src/admin_cmds.c - Diverses commandes pour admins
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
 * $Id: admin_cmds.c,v 1.89 2006/03/15 17:36:47 bugs Exp $
 */

#include "main.h"
#include "outils.h"
#include "cs_cmds.h"
#include "fichiers.h"
#include "vote.h"
#include "add_info.h"
#include "debug.h"
#include "del_info.h"
#include "hash.h"
#include "config.h"
#include "divers.h"
#include "showcommands.h"
#include "aide.h"
#include "timers.h"
#include "template.h"
#include "socket.h"
#include "welcome.h"
#include "version.h"
#include <errno.h>
#include <unistd.h>

int inviteme(aNick *nick, aChan *chan, int parc, char **parv)
{ 	/* invite des admins sur le salon d'infos */
    putserv("%s " TOKEN_INVITE " %s :%s", cs.num, nick->nick, bot.pchan);
    return 1;
}

void CleanUp(void)
{
	aChan *c, *ct;
	anUser *u, *ut;
	aDNR *dnr = dnrhead, *dt;
	struct ignore *ii = ignorehead, *it;
	Timer *t = Timers, *t_t;
	aHashCmd *cmd, *cmd2;
	int i = 0;

	free(cf_quit_msg);
	free(mailprog);
	free(pasdeperm);

	sockets_close();
	purge_network(); /* nicks(+joins) + netchans + servers */

	for(i = 0;i < CHANHASHSIZE;++i) for(c = chan_tab[i];c;c = ct)
	{
		aLink *lp = c->access, *lp_t = NULL;
		aBan *b = c->banhead, *bt = NULL;
		ct = c->next;
		for(; lp; free(lp), lp = lp_t) lp_t = lp->next;
		for(; b; free(b->raison), free(b->mask), free(b), b = bt) bt = b->next;
		free(c->suspend);
		free(c->motd);
		free(c);
	}
	for(i = 0;i < USERHASHSIZE;++i) for(u = user_tab[i];u;u = ut)
	{
		anAccess *a = u->accesshead, *at = NULL;
		aMemo *m = u->memohead, *mt = NULL;
		if(GetConf(CF_MEMOSERV)) for(; m; free(m), m = mt) mt = m->next;
		ut = u->next;
		for(; a; free(a->info), free(a), a = at) at = a->next;
		free(u->suspend);
		free(u->lastlogin);
		free(u);
	}
	for(;dnr;free(dnr->mask), free(dnr->raison), free(dnr), dnr = dt) dt = dnr->next;
	for(; ii; free(ii), ii = it) it = ii->next; /* ignores */
	for(; t; free(t), t = t_t) t_t = t->next; /* timers */

        for(i = 0; i < CMDHASHSIZE; ++i) for(cmd = cmd_hash[i]; cmd; cmd = cmd2)
        {
                int j = 0, h = 0;
                cmd2 = cmd->next;
                for(; j < LangCount ; ++j, h = 0)
                {
                        if(!cmd->help[j]) continue; /* no help (ping etc.) */
                        for(; h < cmd->help[j]->count; ++h) free(cmd->help[j]->buf[h]);
                        free(cmd->help[j]->buf);
                        free(cmd->help[j]);
                }
                free(cmd->help);
                free(cmd);
        }
 	 
        tmpl_clean();
	lang_clean();
}

int die(aNick *nick, aChan *chan, int parc, char **parv)
{
        const char *r = parc ? parv2msg(parc, parv, 1, 300) : cf_quit_msg;

	putserv("%s " TOKEN_QUIT " :%s", cs.num, r);
	putserv("%s " TOKEN_SQUIT " %s 0 :%s", bot.servnum, bot.server, r);

#ifdef USEBSD
	usleep(500000);
#endif
	running = 0;

	return 1;
}

/*
 * restart_bot parv[1->parc-1] = reason
 */
int restart_bot(aNick *nick, aChan *chan, int parc, char **parv)
{
	FILE *fuptime;
	const char *r = parc ? parv2msg(parc, parv, 1, 300) : cf_quit_msg;

	if((fuptime = fopen("uptime.tmp", "w"))) /* permet de maintenir l'uptime (/me tricheur)*/
	{
		fprintf(fuptime, "%lu", bot.uptime);
		fclose(fuptime);
	}

	putserv("%s " TOKEN_QUIT " :%s [\2Redémarrage\2]", cs.num, r);
	putserv("%s "TOKEN_SQUIT" %s 0 :%s [\2Redémarrage\2]", bot.servnum, bot.server, r);

#ifdef USEBSD
	/* attendre 0.5 sec avant de lancer le restart
	 * sinon il n'y aura pas de message de quit sous *BSD
	 */
        usleep(500000);
#endif
        ConfFlag |= CF_RESTART;
        running = 0;

	return 1;
}

int chcomname(aNick *nick, aChan *chan, int parc, char **parv)
{
	aHashCmd *cmd;
	const char *newcmd = parv[2], *lastcmd = parv[1];

	if(strlen(newcmd) > CMDLEN)
		return csreply(nick, "Un nom de commande ne doit pas dépasser %d caractères.", CMDLEN);

	if(!(cmd = FindCommand(lastcmd)) || *cmd->name == '\1')
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
	int level, adm = getoption("-admin", parv, parc, 2, -1), help = getoption("-helpeur", parv, parc, 2, -1);
	aHashCmd *cmdp;

	if(!(cmdp = FindCommand(cmd))) return csreply(nick, GetReply(nick, L_NOSUCHCMD), cmd);

	if(!Strtoint(parv[2], &level, 0, OWNERLEVEL))
		return csreply(nick, "Veuillez préciser un level valide.");

	if(level == cmdp->level && !adm && !help)
		return csreply(nick, "%s est déjà au niveau %d.", cmd, level);

	if(!NeedNoAuthCmd(cmdp) && !level) 
                   return csreply(nick, "La commande %s ne peut être utilisée que par des users"
			" identifiés sur les services. Le niveau minimal est donc de 1.", cmd); 

	if(!ChanCmd(cmdp))
	{
		if(level > MAXADMLVL) return csreply(nick, "Le niveau maximum Administrateur est %d.", MAXADMLVL);
		else if(level > nick->user->level)
			return csreply(nick, GetReply(nick, L_MAXLEVELISYOURS));
		else {
			if(level == 2) cmdp->flag |= CMD_HELPER;
		 	else { 
				cmdp->flag &= ~CMD_HELPER;
				cmdp->flag |= CMD_ADMIN;
			}
			if(level > 2) cmdp->flag |= CMD_ADMIN;
                        else {
				cmdp->flag &= ~CMD_ADMIN;
				cmdp->flag |= CMD_HELPER;
			}
			if(level <= 1) {
                                cmdp->flag &= ~CMD_ADMIN;
                                cmdp->flag &= ~CMD_HELPER;
                        }
		}
	
	}
	if(ChanCmd(cmdp) && adm)
	{	 /* argument de switch admin pour les cmds chans (0-500 <= donc nécessaire)*/
		if(AdmCmd(cmdp)) {
			cmdp->flag &= ~CMD_ADMIN;
			cmdp->flag &= ~CMD_HELPER;
		}
		else {
			cmdp->flag |= CMD_ADMIN;
			cmdp->flag &= ~CMD_HELPER;
		}
	}

	if(ChanCmd(cmdp) && help)
        {        /* argument de switch helpeur pour les cmds chans (0-500 <= donc nécessaire)*/
                if(HelpCmd(cmdp)) {
			cmdp->flag &= ~CMD_HELPER;
			cmdp->flag &= ~CMD_ADMIN;
		}
                else {
			cmdp->flag |= CMD_HELPER;
			cmdp->flag &= ~CMD_ADMIN;
		}
        }

	cmdp->level = level;

	csreply(nick, "Le niveau de la commande %s est maintenant\2 %d\2 (%s).",
				cmdp->name, cmdp->level, AdmCmd(cmdp) ? "Admin" : HelpCmd(cmdp) ? "Helpeur" : "User");
	write_cmds();
	BuildCommandsTable(1);
	return 1;
}

int disable_cmd(aNick *nick, aChan *chan, int parc, char **parv)
{
	const char *cmd = parv[1];
	int i = 0, nb = 0;
	aHashCmd *listcmd, *cmdp = FindCommand(cmd);

	if (!strcasecmp(cmd, "-LIST")) {

        	csreply(nick, "Liste des commandes désactivées:");

        	for(;i < CMDHASHSIZE;++i) for(listcmd = cmd_hash[i];listcmd;listcmd = listcmd->next)
        	{
                	if(listcmd->flag & CMD_DISABLE)
                	{
                        	csreply(nick,"          Level %d - %s", listcmd->level, listcmd);
                        	++nb;
                	}
         	}
        	if(!nb) csreply(nick, "Aucune commande désactivée.");
        	else csreply(nick, "Fin de la liste. %d commande%s désactivée%s.",nb, PLUR(nb), PLUR(nb));
	}
	else {
		if(!cmdp) return csreply(nick, GetReply(nick, L_NOSUCHCMD), cmd);

		if(!strcasecmp(cmdp->corename, "DISABLE"))
				return csreply(nick, "Veuillez ne pas désactiver \2%s\2.", cmd);
		
		switch_option(nick, parv[2], "disable", cmd, &cmdp->flag, CMD_DISABLE);

		BuildCommandsTable(1);
		write_cmds();
	}
	return 1;
}

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

int globals_cmds(aNick *nick, aChan *chan, int parc, char **parv)
{
	const char *cmd = parv[1];
	int i = 0;

	if(!strcasecmp(cmd, "MSG"))
	{
		const char *msg = parv2msg(parc, parv, 2, 300);

		for(; i < CHANHASHSIZE; ++i) for(chan = chan_tab[i]; chan; chan = chan->next)
			if(CJoined(chan) && chan->netchan->users > 1)
				putserv("%s "TOKEN_PRIVMSG" %s :\2%s>\2 %s", cs.num, chan->chan, nick->nick, msg);
	}
	else if(GetConf(CF_MEMOSERV) && (!strcasecmp(cmd, "MEMO")))
	{
		char memo[MEMOLEN + 1];
		anUser *user;
		int min_level = getoption("-min", parv, parc, 2, 1);
		int max_level = getoption("-max", parv, parc, 2, 1);
		int mask = getoption("-user", parv, parc, 2, 0);
		int owner = getoption("-owner", parv, parc, 2, -1), count = 0;

		for(i = 2; i < parc && *parv[i] == '-'; i += 2);

		parv2msgn(parc, parv, i, memo, MEMOLEN);

		if(max_level > MAXADMLVL || min_level > MAXADMLVL)
			return csreply(nick, GetReply(nick, L_VALIDLEVEL));

		for(i = 0; i < USERHASHSIZE; ++i) for(user = user_tab[i]; user; user = user->next)
			if(user != nick->user && (!min_level || user->level >= min_level)
			   && (!max_level || user->level <= max_level)
			   && (!mask || !match(parv[mask], user->nick))
			   && (!owner || IsAnOwner(user)))
			{
				if(!UNoMail(user)) tmpl_mailsend(&tmpl_mail_memo, user->mail, user->nick, NULL, NULL, nick->user->nick, memo);
				if(user->n && !IsAway(user->n))
					csreply(user->n, "\2MEMO Global de %s :\2 %s", nick->user->nick, memo);
				else add_memo(user, nick->user->nick, CurrentTS, memo, MEMO_AUTOEXPIRE);
				++count;
			}

		csreply(nick, "Un memo suivant a été envoyé à %d users: %s", count, memo);
	}

	else if(GetConf(CF_MEMOSERV) && (!strcasecmp(cmd, "CMEMO"))) /* mémo aux accès d'un salon */
	{
		aLink *l;
		char memo[MEMOLEN + 1];
		int level = getoption("-level", parv, parc, 2, 1);

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
				if(!UNoMail(a->user)) tmpl_mailsend(&tmpl_mail_memo, a->user->mail, a->user->nick, NULL, NULL, nick->user->nick, memo);
				if(a->user->n && !IsAway(a->user->n))
					csreply(a->user->n, "\2MEMO Global de %s :\2 %s", nick->user->nick, memo);
				else add_memo(a->user, nick->user->nick, CurrentTS, memo, MEMO_AUTOEXPIRE);
				++i;
			}
		}

		csreply(nick, "Un memo suivant a été envoyé à %d users: %s", i, memo);
	}
	else return csreply(nick, GetReply(nick, L_UNKNOWNOPTION), cmd);
	return 1;
}

int dnr_manage(aNick *nick, int type, int parc, char **parv)
{
	if(!strcasecmp(parv[1], "list"))
	{
		int from = getoption("-from", parv, parc, 2, 0);
		int mask = getoption("-match", parv, parc, 2, 0), count = 0;
		aDNR *dnr = dnrhead;

		for(;dnr;dnr = dnr->next)
			if(dnr->flag & type && (!from || !strcasecmp(parv[from], dnr->from))
				&& (!mask || !match(parv[mask], dnr->mask)))
			{
				csreply(nick, "DNRMask: \2%s\2 Auteur: \2%s\2 Posé le:\2 %s\2 Raison: %s",
					dnr->mask, dnr->from, get_time(nick, dnr->date), dnr->raison);
				++count;
			}
		csreply(nick, "%d matches correspondantes.", count);
		return 0;
	}
	else if(!strcasecmp(parv[1], "add") && parc >= 2)
	{
		aDNR *dnr = find_dnr(parv[2], type | DNR_MASK);
		int both = getoption("-both", parv, parc, 3, -1) ? DNR_TYPECHAN|DNR_TYPEUSER : type;

		if(dnr)
			return csreply(nick, "%s est déjà un DNR mask (Ajouté par %s, le %s [%s])",
				dnr->mask, dnr->from, get_time(nick, dnr->date), dnr->raison);

		add_dnr(parv[2], nick->user->nick,
			parc > (both != type ? 3 : 2)
			? parv2msg(parc, parv, both != type ? 4 : 3, 250) : "Service",
			CurrentTS, HasWildCard(parv[2]) ? both | DNR_MASK : both);
		csreply(nick, "%s a été ajouté aux DNR masks.", parv[2]);
	}
	else if(!strcasecmp(parv[1], "del") && parc >= 2)
	{
		aDNR *dnr = find_dnr(parv[2], type);

		if(!dnr) return csreply(nick, "%s n'est pas un DNR mask.", parv[2]);
		del_dnr(dnr);
		csreply(nick, "DNR mask supprimé.");
	}
	else return csreply(nick, "Syntaxe: %s (ADD|DEL) <dnr-mask> [raison]", parv[0]);
	write_dnr();
	return 0;
}

int dnrchan_manage(aNick *nick, aChan *c, int parc, char **parv)
{
	return dnr_manage(nick, DNR_TYPECHAN, parc, parv);
}

int dnruser_manage(aNick *nick, aChan *c, int parc, char **parv)
{
	return dnr_manage(nick, DNR_TYPEUSER, parc, parv);
}

int rehash_conf(aNick *nick, aChan *chan, int parc, char **parv)
{
        int langs;
        Lang *l;

	if(getoption("-purge", parv, parc, 1, -1))
        {
                check_accounts();
                check_chans();
                return csreply(nick, "Purge effectuée avec succès.");
        }
	if(getoption("-template", parv, parc, 1, -1))
        {
                tmpl_load();
                return csreply(nick, "Actualisation des Templates avec succès.");
        }
	if((langs = getoption("-aide", parv, parc, 1, 0)))
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
	
	if(LangCount != langs)
        {
                help_cmds_realloc(langs);
                for(l = DefaultLang; l; l = l->next) if(l->id > langs) help_load(l);
                csreply(nick, "%d langs and help loaded.", LangCount - langs);
        }

	return csreply(nick, "Configuration actualisée avec succès");
}

int showconfig(aNick *nick, aChan *c, int parc, char **parv)
{
	csreply(nick, "Etat des variables de configuration de SDreams %s", SPVERSION);
	csreply(nick, "Uplink: %s/%s:%d", bot.ip, bot.bindip, bot.port);
	csreply(nick, "Nom du serveur des Services : %s (%s) [Num: %s]", bot.server, bot.name, bot.servnum);
	csreply(nick, "Salon de controle de %s: %s", cs.nick, bot.pchan);
	csreply(nick, "Salon d'aide : %s", bot.chan);
	csreply(nick, "Message de quit : %s", cf_quit_msg);
	csreply(nick, "Message PASDEPERM : %s", pasdeperm);
	csreply(nick, "Message de raison par defaut: %s", defraison);
	csreply(nick, "Temps avant un kill/svsnick : %s", duration(kill_interval));
	csreply(nick, "Max LastSeen : %s", duration(cf_maxlastseen));
	csreply(nick, "Admin exempté de flood : %s - Kill pour flood : %s",
		GetConf(CF_ADMINEXEMPT) ? "Oui" : "Non", GetConf(CF_KILLFORFLOOD) ? "Oui" : "Non");
	if (GetConf(CF_NICKSERV)) csreply(nick, "Nickserv n'utilise pas le kill: %s", GetConf(CF_NOKILL) ? "Oui" : "Non");
	csreply(nick, "Temps d'ignore : %s", duration(ignoretime));
	csreply(nick, "Programme de mail : %s", mailprog ? mailprog : "aucun");
	csreply(nick, "Host hidding: %s (Suffixe: %s)", GetConf(CF_HOSTHIDING) ? "Oui" : "Non", hidden_host);
	if(GetConf(CF_HAVE_CRYPTHOST)) csreply(nick, "Cryptage des Host: Oui - Type: %s", 
		GetConf(CF_CRYPT_MD2_MD5) ? "MD2-MD5" : GetConf(CF_SHA2) ? "SHA + domaine": "SHA + iso");
	else csreply(nick, "Cryptage des Host: Non");
	if(GetConf(CF_MEMOSERV)) csreply(nick, "Nombre de mémos a une meme personne: %d", MAXMEMOS);
	csreply(nick, "Utilisation de la commande %s par les Admins: %s", RealCmd("REGISTER"), GetConf(CF_ADMINREG) ? "Oui" : "Non" );
	if(GetConf(CF_ADMINREG)) csreply(nick, "Les Admins sont les seuls à pouvoir utiliser la commande %s: %s", RealCmd("REGISTER"),
		GetConf(CF_ADMINREGONLY) ? "Oui" : "Non");
	if(GetConf(CF_WELCOMESERV)) csreply(nick, "Le Welcome est envoyer en %s", GetConf(CF_PRIVWELCOME) ? "Privé" : "Notice");
	csreply(nick, "--- Modules Actifs ---");
	csreply(nick, "NickServ: %s", GetConf(CF_NICKSERV) ? "Oui" : "Non");
	csreply(nick, "MemoServ: %s", GetConf(CF_MEMOSERV) ? "Oui" : "Non");
	csreply(nick, "WelcomeServ: %s", GetConf(CF_WELCOMESERV) ? "Oui" : "Non");
	csreply(nick, "VoteServ: %s", GetConf(CF_VOTESERV) ? "Oui" : "Non");
	csreply(nick, "TrackServ: %s", GetConf(CF_TRACKSERV) ? "Oui" : "Non");	
	if(GetConf(CF_WEBSERV)) csreply(nick, "WebServ: Oui - Port: %d", bot.w2c_port);
	else csreply(nick, "WebServ: Non");
	return 1;
}
