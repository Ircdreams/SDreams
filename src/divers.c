/* src/divers.c - Diverses commandes
 * Copyright (C) 2004-2005 ircdreams.org
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
 * $Id: divers.c,v 1.65 2006/03/15 19:04:42 bugs Exp $
 */

#include "main.h"
#include "config.h"
#include "crypt.h"
#include "debug.h"
#include "divers.h"
#include "outils.h"
#include "hash.h"
#include "cs_cmds.h"
#include "admin_manage.h"
#include "admin_user.h"
#include "add_info.h"
#include "template.h"
#include "version.h"
#include <errno.h>

int uptime(aNick *nick, aChan *chan, int parc, char **parv)
{
	return csreply(nick, GetReply(nick, L_UPTIME), duration(CurrentTS - bot.uptime));
}

int ctcp_ping(aNick *nick, aChan *chan, int parc, char **parv)
{
	if(parc) csreply(nick, "\1PING %s", parv[1]);
	return 1;
}

int ctcp_version(aNick *nick, aChan *chan, int parc, char **parv)
{
    csreply(nick, "\1VERSION Services SDreams [" SPVERSION "] © IrcDreams.org (Compilé le " __DATE__ " "__TIME__ ")\1");
	return 1;
}

int version(aNick *nick, aChan *chan, int parc, char **parv)
{
	csreply(nick, "Services SDreams [" SPVERSION "] © IrcDreams.org (Compilé le " __DATE__ " "__TIME__ ")");
        return 1;
}

int lastseen(aNick *nick, aChan *chan, int parc, char **parv)
{
	anUser *u = getuserinfo(parv[1]);

	if(!u) csreply(nick, GetReply(nick, L_NOSUCHUSER), parv[1]);
	else if(u->n && !IsHiding(u->n)) csreply(nick, GetReply(nick, L_ELSEALREADYLOG));
	else csreply(nick, GetReply(nick, L_FULLLASTSEEN), u->nick,
                        duration(CurrentTS - u->lastseen), get_time(nick, u->lastseen));

	return 1;
}

int show_admins(aNick *nick, aChan *chan, int parc, char **parv)
{
	int i = 0;
	anUser *u;
	csreply(nick, "\2Présent  Niveau  Username       Pseudo\2");
	for(;i < USERHASHSIZE;++i) for(u = user_tab[i];u;u = u->next)
		if(IsAdmin(u)) csreply(nick, "\2\003%s\2\3      %d       %-13s  \0032%s\3",
				(u->n && !IsHiding(u->n)) ? (IsAway(u->n) || UIsBusy(u)) ? "14ABS" : "3OUI" : "4NON",
				u->level, u->nick, (u->n && !IsHiding(u->n))  ? u->n->nick : "");
	return 1;
}

int show_helper(aNick *nick, aChan *chan, int parc, char **parv)
{
        int i = 0, nb = 0;
        anUser *u;
        csreply(nick, "\2Présent  Username       Pseudo\2");
        for(;i < USERHASHSIZE;++i) for(u = user_tab[i];u;u = u->next)
                if(u->level == 2) {
			csreply(nick, "\2\003%s\2\3      %-13s  \0032%s\3",
                                (u->n && !IsHiding(u->n)) ? (IsAway(u->n) || UIsBusy(u)) ? "14ABS" : "3OUI" : "4NON",
                                u->nick, (u->n && !IsHiding(u->n)) ? u->n->nick : "");
			++nb;
		}
	if (!nb) csreply(nick,"Aucun Agent d'aide a été défini");
        return 1;
}

int show_ignores(aNick *nick, aChan *chan, int parc, char **parv)
{
	struct ignore *i= ignorehead;
	int c = 0;

	if(!ignorehead) return csreply(nick, GetReply(nick, L_NOINFOAVAILABLE));

	csreply(nick, "Liste des Ignorés:");
	for(;i;i = i->next)
		csreply(nick,"\002%d\2. \2%s\2 Expire dans %s", ++c, GetIP(i->host), duration(i->expire - CurrentTS)); 

	return 1;
}

int verify(aNick *nick, aChan *chan, int parc, char **parv)
{
        aNick *n;

        if(!strcasecmp(parv[1], cs.nick)) return csreply(nick, "Yeah, c'est bien moi :)");
        if(!(n = getnickbynick(parv[1])) || (IsHiding(n) && !IsAnAdmin(nick->user))) return csreply(nick, GetReply(nick, L_NOSUCHNICK), parv[1]);
        if(!n->user) return csreply(nick, GetReply(nick, L_NOTLOGUED), n->nick);

        csreply(nick, "%s est logué sous l'username %s%s%s%s",
                (IsAnAdmin(nick->user) || n == nick) ? GetNUHbyNick(n, 0) : n->nick, n->user->nick,
                IsAdmin(n->user) ? " - Administrateur des Services" : "", IsOper(n) ? " - IRCop" : "",
		IsHelper(n) ? " - Agent d'Aide du Réseau" : "");
        return 1;
}

int swhois(aNick *nick, aChan *chan, int parc, char **parv)
{
        anUser *u;
        char *arg = parv[1], *swhois = parv2msg(parc, parv, 2, SWHOISLEN), memo[MEMOLEN + 1];

        if(strlen(swhois) > SWHOISLEN)
        	return csreply(nick, "La longueur du SWHOIS est limitée à %d caractères.", SWHOISLEN);

	if(!(u = ParseNickOrUser(nick, arg))) return 0;

	if(strcasecmp(u->nick, nick->user->nick) && IsAdmin(u) && u->level >= nick->user->level && nick->user->level < MAXADMLVL)
        	return csreply(nick, "L'user \2%s\2 est un Admin de niveau supérieur au votre.", u->nick);

	if(u->swhois && (!strcmp(swhois, u->swhois))) return csreply(nick, "Ce SWHOIS est déjà mis en place.");

	str_dup(&u->swhois, swhois);

        if(!strcmp(swhois, "none")) {
               	csreply(nick, "Le SWHOIS de %s a été annulé", u->nick);
                DelUSWhois(u);
                if (u->n) {
			putserv("%s " TOKEN_SWHOIS " %s", bot.servnum, u->n->numeric);
			csreply(u->n, "Votre SWHOIS a été annulé par %s", nick->user->nick);
			if(u->swhois) free(u->swhois), u->swhois = NULL;
		}
		snprintf(memo, MEMOLEN, "Votre SWHOIS a été annulé par %s.", nick->user->nick);
        }
        else {
		csreply(nick, "Le SWHOIS de %s (%s) a été mis en place.", u->nick, u->swhois);
               	SetUSWhois(u);
		if (u->n) {
			putserv("%s " TOKEN_SWHOIS " %s :%s", bot.servnum, u->n->numeric, u->swhois);
			csreply(u->n, "Votre SWHOIS (%s) a été mis en place par %s.", u->swhois, nick->user->nick);
		}
		snprintf(memo, MEMOLEN, "Votre SWHOIS (%s) a été mis en place par %s.", u->swhois, nick->user->nick);
	}
	add_memo(u, cs.nick, CurrentTS, memo, MEMO_AUTOEXPIRE);
	if(!UNoMail(u)) tmpl_mailsend(&tmpl_mail_memo, u->mail, u->nick, NULL, NULL, cs.nick, memo);
	return 1;
}

int sethost(aNick *nick, aChan *chan, int parc, char **parv)
{
        anUser *u,*who;
        char *arg = parv[1], *host = parv[2], memo[MEMOLEN + 1];

        if(strlen(host) > HOSTLEN)
               	return csreply(nick, "La longueur du host est limitée à %d caractères.", HOSTLEN);

        if (!IsValidHost(host))
               	return csreply(nick, "Le host n'est pas valide.");

	if(!(u = ParseNickOrUser(nick, arg))) return 0;
	
	if(strcasecmp(u->nick, nick->user->nick) && IsAdmin(u) && u->level >= nick->user->level && nick->user->level < MAXADMLVL)
               	return csreply(nick, "L'user \2%s\2 est un Admin de niveau supérieur au votre.", u->nick);

	if(u->vhost && !strcmp(host, u->vhost)) return csreply(nick, "Ce vhost est déjà mis en place.");

	if((who = GetUserIbyVhost(host))) return csreply(nick, "Ce vhost est déjà utilisé par %s.",who->nick);

        if(!strcmp(host, "none")) {
               	csreply(nick, "Le vhost de %s a été annulé", u->nick);
                DelUVhost(u);
		Strncpy(u->vhost, "none", HOSTLEN);
		hash_delvhost(u);
                if (u->n && UVhost(u)) {
			putserv("%s SVSHOST %s %s.%s", bot.servnum, u->n->numeric, u->nick, hidden_host);
			csreply(u->n, "Votre VHOST a été annulé par %s", nick->user->nick);
			nick->flag = parse_umode(nick->flag, "-H");
		}
		snprintf(memo, MEMOLEN, "Votre VHOST a été annulé par %s.", nick->user->nick);
	}
        else {
		csreply(nick, "Le vhost de %s (%s) a été mis en place.", u->nick, host);
		switch_vhost(u,host);
		DelURealHost(u);
		DelUWantX(u);
                SetUVhost(u);
		if (u->n) {
			putserv("%s SVSHOST %s %s", bot.servnum, u->n->numeric, u->vhost);
			csreply(u->n, "Votre VHOST (%s) a été mise en place par %s.", u->vhost, nick->user->nick);
			nick->flag = parse_umode(nick->flag, "+H");
		}
		snprintf(memo, MEMOLEN, "Votre VHOST (%s) a été mise en place par %s.", u->vhost, nick->user->nick);
	}
	add_memo(u, cs.nick, CurrentTS, memo, MEMO_AUTOEXPIRE);
	if(!UNoMail(u)) tmpl_mailsend(&tmpl_mail_memo, u->mail, u->nick, NULL, NULL, cs.nick, memo);
	return 1;
}

int show_country(aNick *nick, aChan *chan, int parc, char **parv)
{
   struct cntryinfo *cntry;
   char *country = parv[1];

   if(*country && *country == '.') country++;
   cntry = cntryhead;
   while(cntry != NULL)
   {
      if(!strcasecmp(country, cntry->iso))
      {
         return csreply(nick,"Le code de pays \2%s\2 est \2%s\2", country, cntry->cntry);
      }
      cntry = cntry->next;
   }
   return csreply(nick,"Le code de pays \2%s\2 n'a pas été trouvé.", country);
}

int myinfo(aNick *nick, aChan *chan, int parc, char **parv)
{
	return show_userinfo(nick, nick->user, 0, 1);
}
