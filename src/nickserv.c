/* src/nickserv.c - Diverses commandes sur le module nickserv
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
 * $Id: nickserv.c,v 1.137 2006/03/16 07:01:03 bugs Exp $
 */

#include "main.h"
#include "hash.h"
#include "admin_user.h"
#include "admin_manage.h"
#include "chanserv.h"
#include "outils.h"
#include "cs_cmds.h"
#include "add_info.h"
#include "del_info.h"
#include "config.h"
#include "fichiers.h"
#include "aide.h"
#include "vote.h"
#include "memoserv.h"
#include "track.h"
#include "nickserv.h"
#include "welcome.h"
#include "showcommands.h"
#include "crypt.h"
#include "template.h"
#include "version.h"
#include <ctype.h>

/*
 * oubli_pass <login> <mail>
 */
int oubli_pass(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	anUser *user;
	char *passwd;

	if(GetConf(CF_NOMAIL)) return csreply(nick, GetReply(nick, L_CMDDISABLE));

	if(nick->user) return csreply(nick, GetReply(nick, L_LOGUEDIN), nick->user->nick);

	if(!(user = getuserinfo(parv[1]))) return csreply(nick, GetReply(nick, L_NOSUCHUSER), parv[1]);

	if(user->n) return csreply(nick, GetReply(nick, L_ELSEALREADYLOG));

	if(UOubli(user)) return csreply(nick, GetReply(nick, L_CMDALREADYUSED), parv[0]);

	if(strcasecmp(user->mail, parv[2]))
		return csreply(nick, GetReply(nick, L_NOTMATCHINGMAIL), user->nick, parv[2]);

	SetUOubli(user), SetNRegister(nick);
	passwd = create_password(user->passwd);

	if(!tmpl_mailsend(&tmpl_mail_oubli, user->mail, user->nick, passwd, nick->host, NULL, NULL))
		return csreply(nick, "Impossible d'envoyer le mail");

	csreply(nick, GetReply(nick, L_PASS_SENT), user->mail);
	return 0;
}

static int login_report_failed(anUser *user, aNick *nick, const char *raison) 
{ 
        if(IsAdmin(user)) 
        { 
                const char *nuh = GetNUHbyNick(nick, 0); 
                cswall("Admin Login échoué sur \2%s\2 par \2%s\2", user->nick, nuh); 
                putlog(LOG_FAUTH, "LOGIN %s (%s) par %s", user->nick, raison, nuh); 
        } 
        return 0; 
} 

int ns_login(aNick *nick, aChan *chan, int parc, char **parv)
{
	anUser *user = NULL;
	anAccess *a;
	aJoin *j = NULL;
	struct track *track;

	if(!(user = getuserinfo(parv[1]))) return csreply(nick, GetReply(nick, L_NOSUCHUSER), parv[1]);

	if(nick->user)
	{
		csreply(nick, GetReply(nick, L_LOGUEDIN), nick->user->nick);
		return (nick->user != user) ? login_report_failed(user, nick, "Déjà logué") : 0;
	}

	if(!checkpass(parv[2], user))
	{
		csreply(nick, GetReply(nick, L_BADPASS), user->nick);
		return login_report_failed(user, nick, "Bad password");
	}

	if(user->n && strcasecmp(parv[0], RealCmd("recover"))) /* username in use */
	{
		csreply(nick, GetReply(nick, L_ELSEALREADYLOG));
		csreply(user->n, GetReply(user->n, L_ELSETRYTOLOG), nick->nick, RealCmd("set"));
		return login_report_failed(user, nick, "Déjà utilisé");
	}
	if(IsSuspend(user))
	{
		show_ususpend(nick, user);
		return login_report_failed(user, nick, "Suspendu");
	}

	if(GetConf(CF_TRACKSERV) && (track = istrack(user)))
		csreply(track->tracker, "[\2Track\2] %s de %s (%s)", parv[0], user->nick, GetNUHbyNick(nick, 0));

	if(GetConf(CF_NICKSERV) && NHasKill(nick) && (!strcasecmp(nick->nick, user->nick) || checknickaliasbyuser(nick->nick,user))) 
		kill_remove(nick); /* arret du kill */
		
	if(UOubli(user)) DelUOubli(user);
	if(UFirst(user)) {
			csreply(nick, "Bienvenue sur les services SDreams. La liste des commandes est disponible via \2/%s %s",
					cs.nick, RealCmd("showcommands"));
			DelUFirst(user);
	} 

	SetNRegister(nick);	/* pour empecher les register si deauth/drop */
	cs_account(nick, user);
	nick->user->lastseen = CurrentTS;

	csreply(nick, GetReply(nick, L_LOGINSUCCESS), user->nick);

	if(IsAdmin(user))
	{
		csreply(nick, GetReply(nick, L_LOGINADMIN), user->level);
		adm_active_add(nick);
		if(GetConf(CF_WELCOMESERV) && *admin_motd) csreply(nick, GetReply(nick, L_LOGINMOTD), admin_motd);
	}

	else if(GetConf(CF_WELCOMESERV) && *user_motd) csreply(nick, GetReply(nick, L_LOGINMOTD), user_motd);

	if(GetConf(CF_MEMOSERV)) show_notes(nick);

	if(GetConf(CF_VOTESERV) && CanVote(user))
	{
		show_vote(nick);
		csreply(nick, "Pour voter tapez \2/%s %s <n° de la proposition>\2.", cs.nick, RealCmd("voter"));
	}

	/*auto-autoop proposé par BugMaster :)*/
	for(a = user->accesshead;a;a = a->next)
	{
		aChan *c = a->c;
		if(AWait(a) || ASuspend(a) || IsSuspend(c)) continue;

		if((j = GetJoinIbyNC(nick, c->netchan)))
		{
			if(AOwner(a) && CWarned(c)) /* manager logs in */
			{
				cstopic(c, c->deftopic);        /* try to restore topic */
				DelCWarned(a->c);                       /* and cancel the purge */
			}
			a->lastseen = 1; /* mark as on chan */
		}

		if(j) enforce_access_opts(c, nick, a, j);

		if(c->motd) csreply(nick, "\2 [Message du jour] - %s -\2 %s", c->chan, c->motd);
		if(!j && CJoined(c) && CAutoInvite(c) && (HasMode(c->netchan, C_MINV|C_MKEY|C_MUSERONLY)
			|| (HasMode(c->netchan, C_MLIMIT) && c->netchan->users >= c->netchan->modes.limit)))
				putserv("%s " TOKEN_INVITE " %s :%s", cs.num, nick->nick, c->chan);
	}
	return 1;
}

int recover(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	aNick *who, *whoauth = NULL, *whon = NULL;

	if(!nick->user)
	{
		anUser *u;
		if(parc < 2) return syntax_cmd(nick, FindCoreCommand("recover"));
		/* cache if anyone is already logued into this account */
  		if((u = getuserinfo(parv[1])) && u->n != nick) whoauth = u->n;
  		/* then perform standard login */
  		if(!ns_login(nick, chaninfo, parc, parv)) return 0;
		/* now check if anyone else is using my account as nick */
		 if((who = getnickbynick(nick->user->nick)) != nick) whon = who;
	}
	else
	{
		if(parc && strcasecmp(nick->user->nick, parv[1])) /* other nick than his login */
			return csreply(nick, GetReply(nick, L_LOGUEDIN), nick->user->nick);

		if(!strcasecmp(nick->nick, nick->user->nick)) /* already his nick */
			return csreply(nick, GetReply(nick, L_ALREADYYOURNICK), nick->nick);

		if(!(whon = getnickbynick(nick->user->nick))) /* noone use it, use /NICK boring guy!*/
			return csreply(nick, GetReply(nick, L_NICKAVAIABLE), nick->user->nick);
	}

	if(whoauth && (whoauth != whon || !UPKill(nick->user)))
	{ /* personne authée sur le même username, deauthons le. (sauf si on va le killer LUI)*/
		csreply(whoauth, GetReply(nick, L_DEAUTHBYRECOVER), nick->nick);
		cs_account(whoauth, NULL);/* attention au cas whoauth!=NULL && whon!=NULL && whon!=whoauth*/
	}

	if(whon) /* personne possédant le nick == username */
	{
		if(whoauth == whon)
		{
			if(IsAdmin(nick->user)) adm_active_del(whon);
			whon->user = NULL;
		}
		putserv("%s " TOKEN_KILL " %s :%s (Fantôme de %s@%s)", cs.num, whon->numeric, cs.nick, nick->nick, nick->user->nick);
		del_nickinfo(whon->numeric, "recover");
		csreply(nick, GetReply(nick, L_NICKAVAIABLE), nick->user->nick);
	}
	return 1;
}

int myaccess(aNick *nick, aChan *chan, int parc, char **parv)
{
	int c = 0, i = 0, nb = 0;
	anAccess *a;

	if(parc)
	{
		const char *cmd = parv[1];
		/* manage pending access */
                if(!strcasecmp(cmd, "accept") || !strcasecmp(cmd, "refuse"))
                {
                        if(parc < 2) return csreply(nick, "Syntaxe: %s %s <#salon>", parv[0], cmd);

                        if(!(chan = getchaninfo(parv[2])))
                                return csreply(nick, GetReply(nick, L_NOSUCHCHAN), parv[2]);

                        for(a = nick->user->accesshead; a && a->c != chan; a = a->next);

                        if(!a || !AWait(a))
                                return csreply(nick, GetReply(nick, L_YOUNOPROPACCESS), parv[2]);

                        csreply(nick, GetReply(nick, (tolower(*cmd) == 'a') ? L_NOWACCESS : L_PROPREFUSED),
                                a->c->chan);
                        /* act */
                        if(tolower(*cmd) == 'a') a->flag &= ~A_WAITACCESS;
                        else del_access(nick->user, a->c);
                        return 1;
                }
		if(!strcasecmp(cmd, "drop")) /* suppress a valid access */
                {
                        if(parc < 2) return csreply(nick, "Syntaxe: DROP <#salon>");

                        if(!(a = GetAccessIbyUserI(nick->user, getchaninfo(parv[2]))))
                                return csreply(nick, GetReply(nick, L_YOUNOACCESSON), parv[2]);

                        if(AOwner(a)) return csreply(nick, GetReply(nick, L_OWNERMUSTUNREG), parv[2]);

                        del_access(nick->user, a->c);

                        csreply(nick, GetReply(nick, L_OKDELETED), parv[2]);
                        return 1;
                }
	}

	c = getoption("-chan", parv, parc, 1, 0);

	for(a = nick->user->accesshead;a;++i, a = a->next)
	{
		if(AWait(a) || (c && match(parv[c], a->c->chan))) continue;
		csreply(nick, "- %s -", a->c->chan);
		show_accessn(a, nick->user, nick);
		++nb;
	}
	return csreply(nick, GetReply(nick, L_TOTALFOUND), i, PLUR(i), PLUR(i));
}

int deauth(aNick *nick, aChan *chan, int parc, char **parv)
{
   csreply(nick, "Vous venez de vous déloguer de l'UserName \2%s\2", nick->user->nick);
   cs_account(nick, NULL);
   return 1;
}

int user_set(aNick *nick, aChan *chan, int parc, char **parv)
{
	const char *arg = parv[1];

	if(!strcasecmp(arg, "mail"))
	{
		if(parc < 2)
			return csreply(nick, "Syntaxe: %s MAIL <nouveau mail>", parv[0]);
			
		if(strlen(parv[2]) > MAILLEN)
			return csreply(nick, GetReply(nick, L_MAILLENLIMIT), MAILLEN);

		if(!IsValidMail(parv[2]))
			return csreply(nick, GetReply(nick, L_MAIL_INVALID));

		if(!strcasecmp(nick->user->mail, parv[2]))
			return csreply(nick, GetReply(nick, L_IDENTICMAIL), parv[2]);

		if(GetUserIbyMail(parv[2])) return csreply(nick, GetReply(nick, L_MAIL_INUSE));

		switch_mail(nick->user, parv[2]);
		csreply(nick, GetReply(nick, L_OKCHANGED));
	}
	else if(!strcasecmp(arg, "sexe"))
	{
		char *choix = parc < 2 ? NULL : parv[2];
		if(!choix) {
			return csreply(nick, "Votre sexe: %s", UMale(nick->user) ? "Homme" : UFemelle(nick->user) ? "Femme" : "Indéfini");
                }
		else if(!strcasecmp(parv[2], "0")) {
			DelUMale(nick->user);
			DelUFemelle(nick->user);
			return csreply(nick, "Votre sexe est maintenant: Non défini");
		}
		else if(!strcasecmp(parv[2], "1")) {
			SetUMale(nick->user);
			DelUFemelle(nick->user);
			return csreply(nick, "Votre sexe est maintenant: Homme");
		}
		else if(!strcasecmp(parv[2], "2")) {
			DelUMale(nick->user);
			SetUFemelle(nick->user);
			return csreply(nick, "Votre sexe est maintenant: Femme");
		}
		else return csreply(nick, "Syntaxe: SET SEXE <0/1/2>");
	}
	else if(!strcasecmp(arg, "myhost"))
	{
		char *choix = parc < 2 ? NULL : parv[2];
		if(!choix) {
			return csreply(nick, "Vhost: %s (%s), AutoMode X: %s, RealHost: %s",
				UVhost(nick->user) ? "Activé" : "Désactivé", nick->user->vhost, 
				UWantX(nick->user) ? "Activé" : "Désactivé",
				URealHost(nick->user) ? "Activé" : "Désactivé"); 
		}
        	else if(!strcasecmp(parv[2], "0")) {
			DelUWantX(nick->user);
			DelUVhost(nick->user);
			DelURealHost(nick->user);
			return csreply(nick, "Annulation Effectué");
		}
		else if(!strcasecmp(parv[2], "1")) {
			if(!GetConf(CF_XMODE)) return csreply(nick, "Commande désactivée.");
			SetUWantX(nick->user);
			DelUVhost(nick->user);
			DelURealHost(nick->user);
			return csreply(nick, "AutoMode X: Activé");
		}
		else if(!strcasecmp(parv[2], "2")) {
			if(!strcasecmp("none", nick->user->vhost))
				return csreply(nick, "Aucun Vhost n'a été configuré. Veuillez consulter un administrateur");
			DelUWantX(nick->user);
			SetUVhost(nick->user);
			DelURealHost(nick->user);
			return csreply(nick, "Vhost: Activé (%s)", nick->user->vhost);
		}
		else if(!strcasecmp(parv[2], "3")) {
			if(!GetConf(CF_HAVE_CRYPTHOST)) return csreply(nick, "Commande désactivée.");
                        DelUWantX(nick->user);
                        DelUVhost(nick->user);
                        SetURealHost(nick->user);
                        return csreply(nick, "RealHost: Activé");
                }
        	else return csreply(nick, "Syntaxe: SET VIRTUALHOST <0/1/2/3>");
        	return 0;
	}
	else if(!strcasecmp(arg, "swhois"))
	{
		int swhois;
		if (parc < 2 || !Strtoint(parv[2], &swhois, 0, 1))
			return csreply(nick, "Syntaxe: %s SWHOIS <0/1>", parv[0]);
		if(!nick->user->swhois)
		{
			DelUSWhois(nick->user);
			return csreply(nick, "Aucun SWHOIS n'a été configuré. Veuillez consulter un administrateur");
		}
		else if(swhois == 1)
		{
			SetUSWhois(nick->user);
			putserv("%s " TOKEN_SWHOIS " %s :%s", bot.servnum, nick->numeric, nick->user->swhois);
			return csreply(nick, "SWHOIS activée: %s", nick->user->swhois);
		}
		else if(swhois == 0)
		{
			DelUSWhois(nick->user);
			putserv("%s " TOKEN_SWHOIS " %s", bot.servnum, nick->numeric);
			return csreply(nick, "SWHOIS désactivée");
		}
		else return csreply(nick, "Syntaxe: SET SWHOIS <0/1>");
	}
	else if(GetConf(CF_NICKSERV) && !strcasecmp(arg, "protect"))
	{
		int protect;
		if(parc < 2 || !Strtoint(parv[2], &protect, 0, 2))
			return csreply(nick, "Syntaxe: %s PROTECT <0/1/2>", parv[0]);

		nick->user->flag &= ~(U_PKILL | U_PNICK);
		if(protect == 1)
		{
			SetUPNick(nick->user);
			return csreply(nick, GetReply(nick, L_SPNICK), kill_interval);
		}
		else if(protect == 2)
		{
			SetUPKill(nick->user);
			return csreply(nick, GetReply(nick, L_SPKILL), kill_interval);
		}
		else return csreply(nick, GetReply(nick, L_SPNONE));
	}
	else if(GetConf(CF_MEMOSERV) && !strcasecmp(arg, "nomemo"))
		switch_option(nick, parc < 2 ? NULL : parv[2], arg, nick->user->nick, &nick->user->flag, U_NOMEMO);
	else if(!strcasecmp(arg, "pass"))
	{
                if(parc < 3)
                        return csreply(nick, GetReply(nick, L_NEED_PASS_TWICE));

                if(strcmp(parv[2], parv[3]))
                        return csreply(nick, GetReply(nick, L_PASS_CHANGE_DIFF));

                if(strlen(parv[2]) < 7)
                        return csreply(nick, GetReply(nick, L_PASS_LIMIT));

		MD5pass(parv[2], nick->user->passwd); 
                SetUMD5(nick->user); /* mark it as md5 */
		csreply(nick, GetReply(nick, L_OKCHANGED));
	}
	else if(!strcasecmp(arg, "username"))
	{
		aDNR *dnr;

		if(!GetConf(CF_USERNAME)) return csreply(nick, "Commande désactivée.");

                if(parc < 2)
                        return csreply(nick, "Syntaxe: %s USERNAME <nouvel username>", parv[0]);

                if(nick->user->flag & U_ALREADYCHANGE)
                        return csreply(nick, GetReply(nick, L_CMDALREADYUSED), "UserName");

                if(strlen(parv[2]) > NICKLEN)
                        return csreply(nick, GetReply(nick, L_USERLENLIMIT), NICKLEN);

                if(!IsValidNick(parv[2]))
                        return csreply(nick, GetReply(nick, L_USER_INVALID), parv[2]);

                if((dnr = IsBadNick(parv[2])))
                        return csreply(nick, GetReply(nick, L_CANREGDNR), parv[2], dnr->raison);

                if(getuserinfo(parv[2]))
                        return csreply(nick, GetReply(nick, L_ALREADYREG), parv[2]);
                if(strcasecmp(nick->nick, parv[2]) && getnickbynick(parv[2]))
                        return csreply(nick, GetReply(nick, L_UCANTREG_INUSE));

                switch_user(nick->user, parv[2]);
                nick->user->flag |= U_ALREADYCHANGE;
                csreply(nick, GetReply(nick, L_OKCHANGED));
	}
	else if(!strcasecmp(arg, "alias"))
	{
		aDNR *dnr;
		anAlias *alias = nick->user->aliashead;
		int count = 0;

		if(parc < 2 )
                        return csreply(nick, "Syntaxe: %s ALIAS ADD|DEL|LIST <alias>", parv[0]);

		if(!strcasecmp(parv[2], "LIST"))
		{
			csreply(nick, "Voici La liste de vos Alias");
			for(;alias;alias = alias->user_nextalias)
			{
				csreply(nick, "%s", alias->name);
				++count;
			}
			if(!count) return csreply(nick, "Aucun Alias Défini");
			else return csreply(nick, "%d Alias" , count);
		}

		if(!strcasecmp(parv[2], "ADD"))
		{
			if(parc < 3 )
                        	return csreply(nick, "Syntaxe: %s ALIAS ADD <alias>", parv[0]);

			if(strlen(parv[3]) > NICKLEN)
				return csreply(nick, "La longueur de l'UserName est limitée à %d caractères.", NICKLEN);

			for(;alias;alias = alias->user_nextalias)
				++count;

			if(count >= MAXALIAS && !IsAdmin(nick->user))
				return csreply(nick, "Vous avez déjà enregistré suffisament d'alias. (MAX: %d)", MAXALIAS);

			if((dnr = IsBadNick(parv[3])) && !IsAdmin(nick->user))
                                return csreply(nick, "Vous ne pouvez engistrer \2%s\2 pour la raison suivante: \2%s\2", parv[3], dnr->raison);

			if(!IsValidNick(parv[3]))
				return csreply(nick, "%s contient des caractères invalides.", parv[3]);

			if(getuserinfo(parv[3]))
				return csreply(nick, "%s est déjà enregistré.", parv[3]);

			if(strcasecmp(nick->nick, parv[3]) && getnickbynick(parv[3])) 
                        	return csreply(nick, "Ce pseudo est actuellement utilisé par quelqu'un d'autre."); 

			add_alias(nick->user, parv[3]);
			return csreply(nick, "Alias ajouté: %s", parv[3]);
		}
		else if(!strcasecmp(parv[2], "DEL"))
		{
                        if(parc < 3 )
                                return csreply(nick, "Syntaxe: %s ALIAS DEL <alias>", parv[0]);
			if(!checknickaliasbyuser(parv[3],nick->user))
				return csreply(nick, "L'alias %s n'est pas enregistré", parv[3]);
			del_alias(nick->user, parv[3]);
			return csreply(nick, "L'alias %s a bien été effacé", parv[3]);
		}
		else return csreply(nick, "Syntaxe: %s ALIAS ADD|DEL <alias>", parv[0]);
	}
	else if(!strcasecmp(arg, "access"))
	{
		int i = 0;
		anAccess *a, *at;

		if(parc < 2 || !Strtoint(parv[2], &i, 0, 2))
			return csreply(nick, "Syntaxe: %s ACCESS <0,1,2>", parv[0]);

		nick->user->flag &= ~(U_POKACCESS | U_PACCEPT | U_PREJECT);
                switch(i)
                {
                        case 0:
                        nick->user->flag |= U_PREJECT;
                        csreply(nick, GetReply(nick, L_PREFUSE));
                        for(a = nick->user->accesshead; a; a = at)
                        {
                                at = a->next;
                                if(AWait(a)) del_access(nick->user, a->c);
                        }
                        break;

                        case 1:
                        nick->user->flag |= U_POKACCESS;
                        csreply(nick, GetReply(nick, L_PASK), cs.nick, RealCmd("myaccess"));
                        break;

                        case 2:
                        nick->user->flag |= U_PACCEPT;
                        csreply(nick, GetReply(nick, L_PACCEPT));
                        for(a = nick->user->accesshead; a; a = a->next)
                                if(AWait(a)) a->flag &= ~A_WAITACCESS;
                        break;
                }

	}
	else if(!strcasecmp(arg, "lang"))
        {
                Lang *lang;

                if(parc < 2)
                {
                        csreply(nick, GetReply(nick, L_LANGLIST));
                        for(lang = DefaultLang; lang; lang = lang->next) csreply(nick, "%s", lang->langue);
                        return 0;
                }

                if(!(lang = lang_isloaded(parv[2])))
                        return csreply(nick, GetReply(nick, L_NOSUCHLANG), parv[2]);

                nick->user->lang = lang;
                csreply(nick, GetReply(nick, L_NEWLANG));
        }
	else if(GetConf(CF_VOTESERV) && !strcasecmp(arg, "novote")) 
		switch_option(nick, parc < 2 ? NULL : parv[2], arg, 
                	nick->user->nick, &nick->user->flag, U_NOVOTE);
	else if(GetConf(CF_MEMOSERV) && !strcasecmp(arg, "nomail"))
		switch_option(nick, parc < 2 ? NULL : parv[2], arg,
                        nick->user->nick, &nick->user->flag, U_NOMAIL);
	else if(!strcasecmp(arg, "replymsg"))
  	        switch_option(nick, parc < 2 ? NULL : parv[2], arg,
  	        	nick->user->nick, &nick->user->flag, U_PMREPLY);
 	else if(!strcasecmp(arg, "busy") && IsAdmin(nick->user))
		switch_option(nick, parc < 2 ? NULL : parv[2], arg, nick->user->nick, &nick->user->flag, U_ADMBUSY);
	else return csreply(nick, GetReply(nick, L_UNKNOWNOPTION), arg);
	return 1;
}
