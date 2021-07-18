/* src/admin_user.c - commandes admins pour gerer les users
 * Copyright (C) 2004 ircdreams.org
 *
 * contact: bugs@ircdreams.org
 * site web: http://ircdreams.org
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
 * $Id: admin_user.c,v 1.89 2006/03/15 17:36:47 bugs Exp $
 */

#include <sys/time.h>
#include "main.h"
#include "outils.h"
#include "hash.h"
#include "cs_cmds.h"
#include "add_info.h"
#include "del_info.h"
#include "fichiers.h"
#include "divers.h"
#include "crypt.h"
#include "vote.h"
#include "admin_user.h"
#include "config.h"

int cs_whois(aNick *nick, aChan *chan, int parc, char **parv)
{
	char chans[451] = {0}, b[CHANLEN + 4] = {0};
	int size = 0, count = 0, mcount = 0, msize = 0;
	anUser *uptr;
	aJoin *join;
	aNick *nptr = getnickbynick(parv[1]);

	if(!nptr) return csreply(nick, GetReply(nick, L_NOSUCHNICK), parv[1]);

	csreply(nick, "Infos sur \2%s", nptr->nick);
	csreply(nick, " Nom: %s", nptr->name);
	if(GetConf(CF_HAVE_CRYPTHOST)) {
	        csreply(nick, " Mask: %s\2%s\2!\002%s\2@\002%s", GetPrefix(nptr), nptr->nick, nptr->ident, nptr->crypt);
        	csreply(nick, " Realhost: \002%s\2", nptr->host);
	}
	else csreply(nick, " Mask: %s\2%s\2!\002%s\2@\002%s", GetPrefix(nptr), nptr->nick, nptr->ident, nptr->host);
	csreply(nick, " IP:\2 %s", GetIP(nptr->base64));
	if(nptr->user) {
        	if (strcmp("none", nptr->user->vhost)) csreply(nick, " Vhost: %s (%s)", nptr->user->vhost,
			UVhost(nptr->user) ? "Actif" : "Inactif");
        	if (nptr->user->swhois) csreply(nick, " SWhois: %s (%s)", nptr->user->swhois,
			USWhois(nptr->user) ? "Actif" : "Inactif");
	}
	if(IsOper(nptr)) csreply(nick, " %s est \2IRCop.", nptr->nick);
	csreply(nick, " UserModes: \2%s\2", (nptr->flag & N_UMODES) ? GetModes(nptr->flag) : "Aucun");

	for(join = nptr->joinhead;join; join = join->next)
	{
		int tmps = size, p = fastfmt(b, "$$$$ ", IsVoice(join) ? "+" : "", IsHalfop(join) ? "%" : "", IsOp(join) ? "@" : "",join->chan->chan);
		size += p;
		if(size >= 450) /* le dernier fait depasser le buffer */
		{
			csreply(nick, count++ ? "%s ...": " Présent sur %s ...", chans);
			strcpy(chans, b); /* ajoute le suivant pour la prochaine*/
			size = p;
		}
		else strcpy(&chans[tmps], b);
	}

	csreply(nick, !count ? " Présent sur %s" : "%s", *chans ? chans : "aucun salon.");

	if(nptr->user)
	{
		anAccess *acces = nptr->user->accesshead;
		*chans = 0;
		size = count = mcount = msize = 0;

		csreply(nick, " %s est logué sous l'username \2%s\2%s", nptr->nick, nptr->user->nick,
			IsAdmin(nptr->user) ? " (\2Admin\2)" : "");

		for(;acces;acces = acces->next)
		{
			int tmps = size, p;
			if(AWait(acces)) continue;

			size += (p = strlen(acces->c->chan));
			if(size >= 450)
			{
				csreply(nick, !count++ ? "%s ..." : GetReply(nick, L_ACCESSON) , chans);
				size = p;
				strcpy(chans, acces->c->chan); /* ajoute le suivant pour la prochaine*/
			}
			else strcpy(&chans[tmps], acces->c->chan);
			chans[size++] = ' ';
			chans[size] = 0;
		}
		if(*chans) csreply(nick, !count ? GetReply(nick, L_ACCESSON) : "%s", chans);
		else if(!count) csreply(nick, GetReply(nick, L_NOACCESS));
	}
	else if((uptr = getuserinfo(parv[1]))) csreply(nick, " %s est un username enregistré. %s",
				uptr->nick, IsAdmin(uptr) ? " (\2Admin\2)" : "");

	csreply(nick, " Connecté depuis %s sur \2%s", duration(CurrentTS - nptr->ttmco), nptr->serveur->serv);
	return 1;
}

int admin_user(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	char *user = parv[2];
	const char *arg = parv[1];
	int count = 0;
	anUser *u = NULL;
	aNick *n;
	anAlias *alias = NULL;

	if(!strcasecmp(arg, "info"))
	{
		int mail = getoption("-mail", parv, parc, 2, 0);/* critères de recherche */
		int vhost = getoption("-vhost", parv, parc, 2, 0); // vhost
		int swhois = getoption("-swhois", parv, parc, 2, 0); // swhois
		int alias = getoption("-alias", parv, parc, 2, 0);
		int seen = getoption("-seen", parv, parc, 2, 0);
		int date = getoption("-bseen", parv, parc, 2, 1);
		int max = getoption("-count", parv, parc, 2, 1);
		int auth = getoption("-auth", parv, parc, 2, -1);
		int suspend = getoption("-suspend", parv, parc, 2, -1);
		int oubli = getoption("-oubli", parv, parc, 2, -1);
		int first = getoption("-first", parv, parc, 2, -1);
		int last = getoption("-last", parv, parc, 2, 0);
		int nopurge = getoption("-nopurge", parv, parc, 2, -1), found = 0, i = 0;
		time_t seent = 0, datet = 0; 
                if(seent < 0) seent = 0;                  /* convert 'seen' duration to the lower bound TS */ 
                if(seen > 0 && (seent = convert_duration(parv[seen]))) seent = CurrentTS - seent; 
                /* then convert 'bseen' duration to the higher bound TS */ 
                if(date > 0 && (datet = convert_duration(parv[date]))) datet = CurrentTS - datet; 

		if(*parv[2] == '-') user = NULL;
		else if(!HasWildCard(user))
			return (u = ParseNickOrUser(nick, user)) ?
				(IsHelper(nick->user) ? show_userinfo(nick, u, 1, 0) : show_userinfo(nick, u, 1 , 1)) : 0;

		if(!max) max = strcmp("-all", parv[parc]) ? MAXMATCHES : -1;

		for(;i < USERHASHSIZE;++i) for(u = user_tab[i];u;u = u->next)
		{
			if((!mail || !match(parv[mail], u->mail))
			&& (!vhost || !match(parv[vhost], u->vhost))
			&& (!swhois || !match(parv[swhois], u->swhois))
			&& (!seent || u->lastseen >= seent)
			&& (!datet || u->lastseen < datet)
			&& (!auth || u->n)
			&& (!suspend || IsSuspend(u))
			&& (!oubli || UOubli(u))
			&& (!first || UFirst(u))
			&& (!last || (u->lastlogin && !match(parv[last], u->lastlogin)))
			&& (!nopurge || UNopurge(u))
			&& (!user || !match(user, u->nick))
			&& (!alias || checkmatchaliasbyuser(parv[alias], u))
			&& (++found <= max || max < 0))
			{
				if(IsHelper(nick->user)) show_userinfo(nick, u, 1 , 0);
				else show_userinfo(nick, u, 1, 1);
				csreply(nick, "-");
			}
		}
		
		if(found > MAXMATCHES && max == MAXMATCHES)
			csreply(nick, GetReply(nick, L_EXCESSMATCHES), found, MAXMATCHES);
		if(max > 0 && max < found)
			csreply(nick, "Un Total de %d entrées trouvées (%d listées)",     found, max);
		else if(found) csreply(nick, "Un Total de %d entrées trouvés",    found);
		else csreply(nick, "Aucun Username correspondant à\2 %s\2 trouvé.", user ? user : "*");
	}

	else if(!strcasecmp(arg, "match"))
	{
		char *p = parv[2];
		int w = 1, host = 0, nicki = 0, ident = 0, real = 0, join = 0, par = 2;
		int mode = 0, ac = 0, count = 0, serv = 0, nolimit = 0, i = 0, crypt = 0;

		aServer *link = NULL;
		aNChan *nc = NULL;

		if(*p != '+' && *p != '-')
			return csreply(nick, "Syntaxe %s MATCH [+|-]nughCcsam [args..]", parv[0]);


#define ADMUSR_PARSE_MATCH(x)   if(((x) = ++par) <= parc) { if(!w) (x) = -(x); } \
				else err = 1; \
				break; \

		for(;*p;p++)
		{
			char err = 0;
			switch(*p)
			{
				case '+': w = 1; break;
				case '-': w = 0; break;
				case 'h': ADMUSR_PARSE_MATCH(host);
				case 'n': ADMUSR_PARSE_MATCH(nicki);
				case 'u': ADMUSR_PARSE_MATCH(ident);
				case 'g': ADMUSR_PARSE_MATCH(real);
				case 'a': ADMUSR_PARSE_MATCH(ac);
				if(GetConf(CF_HAVE_CRYPTHOST)) {
				case 'C': ADMUSR_PARSE_MATCH(crypt);
				}
				case 'c': 
                                         	if((join = ++par) <= parc) 
                                              	{ 
                                              		if(!(nc = GetNChan(parv[join]))) 
                                              			return csreply(nick, "Le salon %s n'existe pas.", parv[join]); 
          	                              		if(!w) join = -join; 
                                        	} 
                                        	else err = 1; 
	                                	break; 
				case 'm':
						if(++par <= parc)
						{
							mode = parse_umode(0, parv[par]);
							if(!w) mode = -mode;
						}
						else err = 1;
						break;
				case 's':
						if((serv = ++par) <= parc)
						{
							if(!(link = GetLinkIbyServ(parv[serv])))
								return csreply(nick, "Serveur %s inconnu.", parv[serv]);
							if(!w) serv = -serv;
						}
						else err = 1;
						break;
				default:
						err = 1;
			}
			if(err) return csreply(nick, "Syntaxe %s MATCH [+|-]nughCcsam [args..]", parv[0]);
		}

		if(!strcasecmp("-all", parv[parc])) nolimit = 1;

		for(; i < NICKHASHSIZE; ++i) for(n = nick_tab[i]; n; n = n->next)
			if((!nicki || (nicki > 0 && !match(parv[nicki], n->nick)) || (nicki < 0 && match(parv[-nicki], n->nick)))
				&& (!ident || (ident > 0 && !match(parv[ident], n->ident)) || (ident < 0 && match(parv[-ident], n->ident)))
				&& (!real || (real > 0 && !match(parv[real], n->name)) || (real < 0 && match(parv[-real], n->name)))
				&& (!serv || (serv > 0 && link == n->serveur) || (serv < 0 && link != n->serveur))
				&& (!host || (host > 0 && !match(parv[host], n->host)) || (host < 0 && match(parv[-host], n->host)))
				&& (!mode || (mode > 0 && (n->flag & mode) == mode) || (mode < 0 && !(n->flag & -mode)))
				&& (!ac || (n->user && ((ac > 0 && !match(parv[ac], n->user->nick)) || (ac < 0 && match(parv[-ac], n->user->nick)))))
				&& ((GetConf(CF_HAVE_CRYPTHOST) && ((!crypt || (crypt > 0 && !match(parv[crypt], n->crypt))
					|| (crypt < 0 && match(parv[-crypt], n->crypt))))))
				&& (!join || (join > 0 && GetJoinIbyNC(n, nc)) || (join < 0 && !GetJoinIbyNC(n, nc)))
				&& (++count <= MAXMATCHES || nolimit))
					csreply(nick, "Matching Nick: [%d] \2%s%s\2 -> %s@%s", count, GetPrefix(n), n->nick, n->ident, n->host);

		if(count > MAXMATCHES && !nolimit)
			csreply(nick, "[%d] Trop de nicks trouvés, restreignez votre recherche ou forcez avec '-all' en DERNIER argument.", MAXMATCHES);

		csreply(nick, "Fin de la Recherche: %d matche%s trouvée%s.", count, PLUR(count), PLUR(count));
	}

	else if(!strcasecmp(arg, "suspend"))
	{
		time_t timeout = 0; 
                char *ptr = NULL; 

		if(!(u = ParseNickOrUser(nick, user))) return 0;

		if(nick->user == u) return csreply(nick, "Vous ne pouvez pas vous suspendre.");

		if(IsAdmin(u) && u->level >= nick->user->level)
			return csreply(nick, GetReply(nick, L_GREATERLEVEL), u->nick);
		if(parc >= 3) /* do more parsing */
		{

			if(*parv[3] == '%' && (timeout = convert_duration(++parv[3])) <= 0)
				return csreply(nick, GetReply(nick, L_INCORRECTDURATION));
			   ptr = parv2msg(parc, parv, timeout ? 4 : 3, 250); 
                } 
		switch(handle_suspend(&u->suspend, nick->user->nick, ptr, timeout))
		{
			case -1:
				return csreply(nick, "Veuillez préciser une raison pour suspendre cet UserName.");
			case 1:
				if((n = u->n))
				{
					csreply(n, "Votre Username vient d'être suspendu par l'Admin %s.", nick->nick);
					cs_account(n, NULL);
				}
		}
		/* report.. */ 
                if(IsSuspend(u)) show_ususpend(nick, u); 
                else csreply(nick, "L'Username \2%s\2 n'est plus suspendu.", u->nick); 
	}

	else if(!strcasecmp(arg, "cantregchan"))
	{
		time_t timeout = 0;

		if(!strcasecmp(nick->user->nick, user))
			return csreply(nick, "Vous ne voulez plus pouvoir enregistrer de chan ?");

		if(!(u = ParseNickOrUser(nick, user))) return 0;

		if(IsAdmin(u))
			return csreply(nick, "Vous ne pouvez pas empecher un Admin d'enregistrer des salons.");

		if(parc >= 3 && *parv[3] == '%' && (timeout = convert_duration(++parv[3])) <= 0)
			return csreply(nick, GetReply(nick, L_INCORRECTDURATION));

		if(!CantRegChan(u) || timeout)
		{
			if((u->cantregchan = timeout ? CurrentTS + timeout : 0))
				csreply(nick, "%s ne peut pas enregistrer de salon jusqu'au %s.",
					 u->nick, get_time(nick,u->cantregchan));
			else csreply(nick, "%s ne peut pas enregistrer de salon.", u->nick);
		}
		else
		{
			u->cantregchan = -1;
			csreply(nick, "L'Username \2%s\2 peut maintenant enregistrer des salons.", u->nick);
		}
	}

	else if(!strcasecmp(arg, "mail"))
	{
		if(parc < 3 || !IsValidMail(parv[3]))
			return csreply(nick, GetReply(nick, L_MAIL_INVALID));

		if(!(u = ParseNickOrUser(nick, user))) return 0;

		switch_mail(u, parv[3]);
		csreply(nick,  GetReply(nick, L_OKCHANGED));
	}

	else if(!strcasecmp(arg, "newpass"))
	{
		if(parc < 3 || strlen(parv[3]) < 6) return csreply(nick, GetReply(nick, L_PASS_LIMIT));

		if(!(u = ParseNickOrUser(nick, user))) return 0;

		if(IsAdmin(u) && u->level >= nick->user->level && u != nick->user)
			return csreply(nick, GetReply(nick, L_GREATERLEVEL), u->nick);

		MD5pass(parv[3], u->passwd); 
                SetUMD5(u); /* mark it as md5 */ 
		csreply(nick, GetReply(nick, L_OKCHANGED));
	}

	else if(!strcasecmp(arg, "newnick"))
	{
		if(parc < 3 ) return csreply(nick, "Syntaxe: %s NEWNICK <Pseudo|%%Username> <nouveau UserName>", parv[0]);

		if(strlen(parv[3]) > NICKLEN || !IsValidNick(parv[3]))
			return csreply(nick, GetReply(nick, L_USER_INVALID), parv[3]);
		
		if(!(u = ParseNickOrUser(nick, user))) return 0;

		if(getuserinfo(parv[3])) return csreply(nick, GetReply(nick, L_ALREADYREG), parv[3]);

		if(IsAdmin(u) && u->level >= nick->user->level && u != nick->user)
			return csreply(nick, "Vous ne pouvez pas changer l'username d'un Admin de niveau supérieur ou égal.");

		switch_user(u, parv[3]);
		csreply(nick, GetReply(nick, L_OKCHANGED));
	}
	else if(!strcasecmp(arg, "nopurge")) 
        { 
                if(!(u = ParseNickOrUser(nick, user))) return 0; 
    
                switch_option(nick, parc < 3 ? NULL : parv[3], arg, u->nick, &u->flag, U_NOPURGE); 
        }
	else if(!strcasecmp(arg, "del"))
	{
		if(!strcasecmp(nick->user->nick, user))
			return csreply(nick, "Vous ne devriez pas vous deluser.. (Utilisez la commande \2%s\2)", RealCmd("drop"));

		if(!(u = ParseNickOrUser(nick, user))) return 0;

		if(u->level == MAXADMLVL) return csreply(nick, "%s est un Administrateur de niveau maximum !", user);

		if(IsAdmin(u) && u->level >= nick->user->level)
			return csreply(nick, GetReply(nick, L_GREATERLEVEL), u->nick);

		if((n = u->n))
		{
			cs_account(n, NULL);
			csreply(n, "Votre Username vient d'être supprimé par l'Administrateur %s (%s)",
				nick->nick, (parc > 2) ? parv2msg(parc, parv, 3, 200) : "Aucune raison");
		}

		csreply(nick, "L'username %s a bien été supprimé.", u->nick);

		if(IsAnOwner(u))
		{
			char tmp[200];
			snprintf(tmp, sizeof tmp, "Suppression de l'username de l'owner (%s) par %s",
				u->nick, nick->user->nick);
			del_regnick(u, 0, tmp);
		}
		else del_regnick(u, 0, NULL);
	}
	else if(!strcasecmp(arg, "host"))
        {
		if(parc < 3) return csreply(nick, "Syntaxe: %s HOST <Pseudo|%%UserName> <type 0/1/2/3>", parv[0]);
                if(!(u = ParseNickOrUser(nick, user))) return 0;
		else if(!strcasecmp(parv[3], "0")) {
                        DelUWantX(u);
                        DelUVhost(u);
			DelURealHost(u);
                        csreply(nick, "Annulation Effectué pour %s", u->nick);
                }
                else if(!strcasecmp(parv[3], "1")) {
			if(!GetConf(CF_XMODE)) return csreply(nick, "Commande désactivée.");
                        SetUWantX(u);
                        DelUVhost(u);
			DelURealHost(u);
                        csreply(nick, "AutoMode X: Activé pour %s", u->nick);
                }
                else if(!strcasecmp(parv[3], "2")) {
                        if(!strcmp("none", u->vhost))
                        	return csreply(nick, "Configurez un Vhost en premier.");
                        DelUWantX(u);
                        SetUVhost(u);
			DelURealHost(u);
                        csreply(nick, "Vhost: Activé (%s) pour %s", u->vhost, u->nick);
                }
		else if(!strcasecmp(parv[3], "3")) {
			if(!GetConf(CF_HAVE_CRYPTHOST)) return csreply(nick, "Commande désactivée.");
			DelUWantX(u);
  	                DelUVhost(u);
  	                SetURealHost(u);
  	                csreply(nick, "RealHost: Activé pour %s", u->nick);
		}
                else return csreply(nick, "Syntaxe: %s HOST <Pseudo|%%UserName> <type 0/1/2/3>", parv[0]);
        }
	else if(!strcasecmp(arg, "swhois"))
        {
		if(parc < 3) return csreply(nick, "Syntaxe: %s SWHOIS <Pseudo|%%UserName> <0/1>", parv[0]);
		if(!(u = ParseNickOrUser(nick, user))) return 0;
                if(!u->swhois) {
			DelUSWhois(u);
                        return csreply(nick, "Configurez un SWHOIS en premier");
                }
                else if(!strcasecmp(parv[3], "1"))
                {
                        SetUSWhois(u);
			if(u->n) putserv("%s " TOKEN_SWHOIS " %s :%s", bot.servnum, u->n->numeric, u->swhois);
                        return csreply(nick, "SWHOIS activée: %s pour %s", u->swhois, u->nick);
                }
                else if(!strcasecmp(parv[3], "0"))
                {
                        DelUSWhois(u);
			if(u->n) putserv("%s " TOKEN_SWHOIS " %s", bot.servnum, u->n->numeric);
                        return csreply(nick, "SWHOIS désactivée pour %s", u->nick);
                }
                else return csreply(nick, "Syntaxe: %s SWHOIS <Pseudo|%%UserName> <0/1>", parv[0]);
        }
	else if(!strcasecmp(arg, "alias"))
	{
		if(parc < 3) return csreply(nick, "Syntaxe: %s ALIAS <Pseudo|%%UserName> <ADD|DEL|LIST> <alias>", parv[0]);
		if(!(u = ParseNickOrUser(nick, user))) return 0;
		if(!strcasecmp(parv[3], "ADD"))
		{
			if(parc < 4) return csreply(nick, "Syntaxe: %s ALIAS <Pseudo|%%UserName> <ADD|DEL|LIST> <alias>", parv[0]);
			if(strlen(parv[4]) > NICKLEN)
                                return csreply(nick, "La longueur de l'UserName est limitée à %d caractères.", NICKLEN);

                        for(;alias;alias = alias->user_nextalias)
                                ++count;

                        if(!IsValidNick(parv[4]))
                                return csreply(nick, "%s contient des caractères invalides.", parv[4]);

                        if(getuserinfo(parv[4]))
                                return csreply(nick, "%s est déjà enregistré.", parv[4]);

                        if(strcasecmp(nick->nick, parv[4]) && getnickbynick(parv[4]))
                                return csreply(nick, "Ce pseudo est actuellement utilisé par quelqu'un d'autre.");

                        add_alias(u, parv[4]);
                        return csreply(nick, "Alias ajouté pour %s: %s", u->nick, parv[4]);
		}
		else if (!strcasecmp(parv[3], "DEL"))
		{
			if(parc < 4) return csreply(nick, "Syntaxe: %s ALIAS <Pseudo|%%UserName> <ADD|DEL|LIST> <alias>", parv[0]);
			if(!checknickaliasbyuser(parv[4],u))
                                return csreply(nick, "L'alias %s n'est pas enregistré", parv[4]);
			del_alias(u, parv[4]);
                	return csreply(nick, "L'alias %s de %s a bien été effacé", parv[4], u->nick);
		}
		else if (!strcasecmp(parv[3], "LIST"))
		{
			csreply(nick, "Voici La liste des Alias de %s", u->nick);
                        for(alias = u->aliashead;alias;alias = alias->user_nextalias)
                        {
                                csreply(nick, "%s", alias->name);
                                ++count;
                        }
                        if(!count) return csreply(nick, "Aucun Alias Défini");
                        else return csreply(nick, "%d Alias" , count);
		}
		return csreply(nick, "Syntaxe: %s ALIAS <Pseudo|%%UserName> <ADD|DEL|LIST> <alias>", parv[0]);
	}
	else if(!strcasecmp(arg, "userset"))
	{
		time_t reg;
		char *p = NULL;
		if(!(u = ParseNickOrUser(nick, user))) return 0;
		if(parc < 3 || (reg = strtol(parv[3], &p, 10)) <= 0 || (p && *p) || reg > CurrentTS)
			return csreply(nick, "TS non valide");
		u->reg_time = reg;
		csreply(nick, "Date d'enregistrement de %s mise à %s", u->nick, get_time(nick, u->reg_time));
	}
	else return csreply(nick, GetReply(nick, L_UNKNOWNOPTION), arg);
	return 1;
}

void show_ususpend(aNick *nick, anUser *user)
{
	if(!user->suspend) return; /* on aurait jamais du l'envoyer là*/
	if(IsSuspend(user))
	{
		char buf[TIMELEN + 1] = {0};
		Strncpy(buf, get_time(nick, user->suspend->debut), sizeof buf - 1);
		csreply(nick, "L'UserName %s \2EST\2 suspendu par %s (le %s). Expire %s", 
                        user->nick, user->suspend->from, buf,
			user->suspend->expire ? get_time(nick, user->suspend->expire) : "Jamais");

	}
	else csreply(nick, GetReply(nick, L_UWASSUSPEND), user->nick, user->suspend->from);
	csreply(nick, "Raison: %s", user->suspend->raison);
}

int show_userinfo(aNick *nick, anUser *user, int flag, int all)
{
	anAccess *a;
	anAlias *alias;
	char chans[451] = {0}, chansw[451] = {0}, b[REGCHANLEN + 20];
	int size = 0, counta = 0, sizew = 0, countw = 0, countalias = 0;

	csreply(nick, GetReply(nick, L_INFO_ABOUT), user->nick);
	csreply(nick, GetReply(nick, L_MAILIS), user->mail, UNDEF(user->reg_time));

	if(UFirst(user)) csreply(nick, GetReply(nick, L_UREGTIME), duration(CurrentTS - user->lastseen));
	else {
		if(all) csreply(nick, GetReply(nick, L_LASTLOGINTIME), duration(CurrentTS - user->lastseen), 
			(user->lastlogin ? user->lastlogin : "<unknown>"));
		else csreply(nick, "Dernier login il y a %s.", duration(CurrentTS - user->lastseen));
	}
	if(IsAdmin(user)) csreply(nick, GetReply(nick, L_USERISADMIN), user->level);
	if(IsAnHelper(user)) csreply(nick, "Agent d'aide du réseau.");

	for(a = user->accesshead;a;a = a->next)
	{
		int p = mysnprintf(b, sizeof b, "%s (%d) ", a->c->chan, a->level);
		if(AWait(a))
		{
			int tmps = sizew;
			sizew += p;
			if(sizew >= sizeof chansw -1) /* le dernier fait depasser le buffer */
			{
				csreply(nick, countw++ ? "%s ..." : GetReply(nick, L_PROPACCESS) , chansw);
				strcpy(chansw, b); /* ajoute le suivant pour la prochaine*/
				sizew = p;
			}
			else strcpy(&chansw[tmps], b);
		}
		else
		{
			int tmps = size;
			size += p;
			if(size >= sizeof chans -1) /* le dernier fait depasser le buffer */
			{
				csreply(nick, counta++ ? "%s ..." : GetReply(nick, L_ACCESSON), chans);
				strcpy(chans, b); /* ajoute le suivant pour la prochaine */
				size = p;
			}
			else strcpy(&chans[tmps], b);
		}
	}

	if(*chans) csreply(nick, !counta ? GetReply(nick, L_ACCESSON) : "%s", chans);
	else if(!counta) csreply(nick, GetReply(nick, L_NOACCESS));
	if(*chansw) csreply(nick, !countw ? GetReply(nick, L_PROPACCESS) : "%s", chansw);
	else if(!countw) csreply(nick, GetReply(nick, L_NOPROPACCESS));

	if(flag && user->n) {
		if(all) csreply(nick, " %s%s est actuellement logué sous cet Username (%s@%s)", 
			GetPrefix(user->n), user->n->nick, user->n->ident, user->n->host);
		else csreply(nick, " %s%s est actuellement logué sous cet Username", GetPrefix(user->n), user->n->nick);
	}
	else if(flag) csreply(nick, " Personne n'est actuellement logué sous cet username");

	if(user->suspend) show_ususpend(nick, user);
	if(user->cantregchan > 0) 
                csreply(nick, GetReply(nick, L_CANTREGCHAN), get_time(nick, user->cantregchan)); 
	csreply(nick, GetReply(nick, L_OPTIONS), GetUserOptions(user));
	if (!strcmp("none", user->vhost)) csreply(nick, " Vhost défini: Aucun");
	else csreply(nick, " Vhost défini: %s (%s)", user->vhost, UVhost(user) ? "Actif" : "Inactif");
	if (!user->swhois) csreply(nick, " SWhois défini: Aucun");
	else csreply(nick, " SWhois défini: %s (%s)", user->swhois, USWhois(user) ? "Actif" : "Inactif");

	for(alias = user->aliashead;alias;alias = alias->user_nextalias)
	{
                ++countalias;
	        csreply(nick, "Alias défini: %d) %s", countalias, alias->name);
        }
	if (!countalias) csreply(nick, "Alias défini: Aucun");

	if(GetConf(CF_VOTESERV) && !flag && CanVote(user))
		csreply(nick, " Vous n'avez pas encore voté pour: %s", vote[0].prop);
	return 0;
}
