 /* src/statserv.c
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
 * $Id: statserv.c,v 1.4 2006/03/08 00:50:17 bugs Exp $
 */

#include "main.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include "config.h"
#include "hash.h"
#include "outils.h"
#include "version.h"
#include "webserv.h"

/* Statserv */

int w2c_userbyserv(WClient *cl, int parc, char **parv)
{
	aServer *link = NULL;
	aNick *n;
	int i = 0, usercount = 0;

        if(!(link = GetLinkIbyServ(parv[1])))
        {
                w2c_sendrpl(cl, "Aucun");
                return w2c_exit_client(cl, "OK");
        }
	for(;i < NICKHASHSIZE;++i)
		for(n = nick_tab[i];n;n = n->next)
			if(n->serveur == link)
				usercount++;

        w2c_sendrpl(cl, "%d Utilisateur%s", usercount, PLUR(usercount));
        return w2c_exit_client(cl, "OK");
}

int w2c_online(WClient *cl, int parc, char **parv)
{
	aServer *link = NULL;
	if(!(link = GetLinkIbyServ(parv[1])))
	{
		w2c_sendrpl(cl, "%s Hors Ligne", parv[1]);
		return w2c_exit_client(cl, "OK");
	}
	w2c_sendrpl(cl, "%s En Ligne", link);
	return w2c_exit_client(cl, "OK");
}

int w2c_user_online(WClient *cl, int parc, char **parv)
{
	const char *usr = parv[1];
	anUser *u = NULL;
	if((getnickbynick(usr)) || ((u = getuserinfo(usr)) && u->n)) w2c_sendrpl(cl, "1");
        else w2c_sendrpl(cl, "0");
	return w2c_exit_client(cl, "OK");
}

int w2c_stats(WClient *cl, int parc, char **parv)
{
	const char *cmd = parv[1];
	int i = 0, countserv = 0, ni=0, usra=0, usr=0, chs=0, realchan= 0, realhp = 0, hp = 0, oper = 0;
	anUser *u = NULL;
	aChan *chaninfo;
	aNick *n;
	aNChan *nchan;

	if(!strcasecmp(cmd, "SERV"))
        {
                for(;i < MAXNUM;++i)
                        if(serv_tab[i])
				++countserv;
		w2c_sendrpl(cl, "%d Serveurs connectés", countserv);
        }
	if(!strcasecmp(cmd, "CLIENT"))
	{
		for(;i < NICKHASHSIZE;++i)
			for(n = nick_tab[i];n;n = n->next)
				++ni;
		w2c_sendrpl(cl, "%d Chatteurs connectés", ni);
	}
	if(!strcasecmp(cmd, "USER"))
	{	
		for(;i < USERHASHSIZE;++i)
                        for(u = user_tab[i];u;u = u->next)
                                if(u->n) ++usra;
		w2c_sendrpl(cl, "%d Utilisateurs logués", usra);
	}
	if(!strcasecmp(cmd, "ACCOUNT"))
        {
		for(;i < USERHASHSIZE;++i)
			for(u = user_tab[i];u;u = u->next)
				++usr;
		w2c_sendrpl(cl, "%d Utilisateurs enregistrés", usr);
	}
	if(!strcasecmp(cmd, "CHAN"))
        {
		for(;i< CHANHASHSIZE;++i)
			for(chaninfo = chan_tab[i];chaninfo;chaninfo = chaninfo->next)
				++chs;
		 w2c_sendrpl(cl, "%d Salons enregistrés", chs);
        }
	if(!strcasecmp(cmd, "REALCHAN"))
	{
		for(;i < NCHANHASHSIZE;++i)
			for(nchan = nchan_tab[i];nchan;nchan = nchan->next)
				++realchan;
		w2c_sendrpl(cl, "%d Salons formé", realchan);
	}
	if(!strcasecmp(cmd, "UPTIME"))
	{
		w2c_sendrpl(cl , "%s", duration(CurrentTS - bot.uptime));
	}
	if(!strcasecmp(cmd, "VERSION"))
        {
		w2c_sendrpl(cl, "Services SDreams %s", SPVERSION);
	}
	if(!strcasecmp(cmd, "REALHELPER"))
	{
		for(;i < USERHASHSIZE;++i) for(u = user_tab[i];u;u = u->next)
                	if(u->level == 2 && u->n) ++realhp;
		w2c_sendrpl(cl, "%d Helpeur%s en ligne", realhp, PLUR(realhp));
	}
	if(!strcasecmp(cmd, "HELPER"))
        {
                for(;i < USERHASHSIZE;++i) for(u = user_tab[i];u;u = u->next)
                        if(u->level == 2) ++hp;
                w2c_sendrpl(cl, "%d Helpeur%s", hp, PLUR(hp));
        }
	if(!strcasecmp(cmd, "IRCOP"))
        {
                for(;i < NICKHASHSIZE;++i)
                        for(n = nick_tab[i];n;n = n->next)
                                if(IsOper(n) && !IsService(n)) ++oper;
                w2c_sendrpl(cl, "%d Ircop%s en ligne", oper, PLUR(oper));
        }
	if(!strcasecmp(cmd, "SERVICE"))
        {
                for(;i < NICKHASHSIZE;++i)
                        for(n = nick_tab[i];n;n = n->next)
                                if(IsService(n)) ++oper;
                w2c_sendrpl(cl, "%d Service%s en ligne", oper, PLUR(oper));
        }
	if(!strcasecmp(cmd, "MAXUSER"))
        {
                w2c_sendrpl(cl, "%d Chatteurs", nbmaxuser);
        }

	return w2c_exit_client(cl, "OK");
}

int w2c_whoison(WClient *cl, int parc, char **parv)
{
	aNChan *netchan;

        if(!(netchan = GetNChan(parv[1])) || !netchan->users) {
		w2c_sendrpl(cl, "Aucun Utilisateur");
		return w2c_exit_client(cl, "OK");
	}
        w2c_sendrpl(cl, "%d Utilisateur%s", netchan->users, PLUR(netchan->users));
	return w2c_exit_client(cl, "OK");
}

static char *w2c_GetChanPrefix(aNick *nick, aJoin *join)
{
	static char pref[23];

	if(IsOp(join)) pref[0] = '@';
	else if(IsHalfop(join)) pref[0] = '%';
	else if(IsVoice(join)) pref[0] = '+';
	else pref[0] = 0;
	return pref;
}

int w2c_liston(WClient *cl, int parc, char **parv)
{
	aNChan *netchan;
	aChan *chan = getchaninfo(parv[1]);
	aLink *lp;

        if(!(netchan = GetNChan(parv[1])) || !(lp = netchan->members)) {
                w2c_sendrpl(cl, "Salon Vide");
                return w2c_exit_client(cl, "OK");
        }
	if (chan && CJoined(getchaninfo(parv[1]))) w2c_sendrpl(cl, "@%s", cs.nick);

	for(;lp;lp = lp->next)
	{
		if(IsHiding(lp->value.j->nick)) continue;
		w2c_sendrpl(cl, "%s%s", w2c_GetChanPrefix(lp->value.j->nick, lp->value.j), lp->value.j->nick->nick);
	}
	return w2c_exit_client(cl, "OK");
}

int w2c_topic(WClient *cl, int parc, char **parv)
{
	aNChan *netchan;

        if(!(netchan = GetNChan(parv[1]))) {
                w2c_sendrpl(cl, "Salon Vide");
                return w2c_exit_client(cl, "OK");
        }

	w2c_sendrpl(cl, "%s", netchan->topic);
	return w2c_exit_client(cl, "OK");
}

static char* GetXHost(anUser *user)
{
	static char host[HOSTLEN+1];
	fastfmt(host, "$.$", user->nick, hidden_host);
	return host;
}

int w2c_whois(WClient *cl, int parc, char **parv)
{
	aNick *n;
	aJoin *join;
        char chans[451] = {0},  b[CHANLEN + 4] = {0};
	 int size = 0;

        if(!(n = getnickbynick(parv[1]))) w2c_sendrpl(cl, "erreur Aucune information disponible");
	else {
		w2c_sendrpl(cl, "=nick=%s ident=%s rhost=%s chost=%s xhost=%s vhost=%s umode=%d "
			"since=%d serv=%s",
			n->nick, n->ident, n->host, n->crypt, n->user ? GetXHost(n->user) : "",
			(n->user && strcasecmp(n->user->vhost,"none") && UVhost(n->user)) ? n->user->vhost : "",
			n->flag, n->ttmco, 
			n->serveur->serv);
		if(n->user && n->user->swhois && USWhois(n->user)) w2c_sendrpl(cl, "swhois %s", n->user->swhois);
		w2c_sendrpl(cl, "rname %s", n->name);	
		for(join = n->joinhead;join; join = join->next)
		{
			int tmps = size, p;
			if(join->chan->modes.modes & (C_MSECRET|C_MPRIVATE)) continue;
			p= fastfmt(b, "$$$$ ", IsVoice(join) ? "+" : "", IsHalfop(join) ? "%" : "", IsOp(join) ? "@" : "",join->chan->chan);
			size += p;
                	strcpy(&chans[tmps], b);
		}
		w2c_sendrpl(cl, "join %s", chans);
	}
	return w2c_exit_client(cl, "OK");
}

static const struct Modes {
        char mode;
        int value;
} CMode[] = {
        { 'n', C_MMSG },
        { 't', C_MTOPIC },
        { 'i', C_MINV },
        { 'l', C_MLIMIT },
        { 'k', C_MKEY },
        { 's', C_MSECRET },
        { 'p', C_MPRIVATE },
        { 'm', C_MMODERATE },
        { 'c', C_MNOCTRL },
        { 'C', C_MNOCTCP },
        { 'O', C_MOPERONLY },
        { 'r', C_MUSERONLY },
        { 'R', C_MACCONLY },
        { 'N', C_MNONOTICE },
        { 'q', C_MNOQUITPARTS },
        { 'D', C_MAUDITORIUM },
        { 'T', C_MNOAMSG },
        { 'M', C_MNOCAPS },
        { 'W', C_MNOWEBPUB },
        { 'P', C_MNOCHANPUB },
};

static char 
*w2c_GetCModes(struct cmode modes)
{
        static char mode[ASIZE(CMode) + KEYLEN + 20];
        unsigned int i = 0, j = 0;
        for(;i < ASIZE(CMode);++i) if(modes.modes & CMode[i].value) mode[j++] = CMode[i].mode;
        mode[j] = '\0';
        return mode;
}

int w2c_list(WClient *cl, int parc, char **parv)
{
	int i = 0;
	char *nb = parv[1];
	aNChan *nchan;

	if(!is_num(nb)) w2c_sendrpl(cl, "erreur");
	else {
	for(;i < NCHANHASHSIZE;++i)
        	for(nchan = nchan_tab[i];nchan;nchan = nchan->next)
			if((nchan->users >= atoi(nb)) && (!(nchan->modes.modes & (C_MSECRET|C_MPRIVATE)))) {
				w2c_sendrpl(cl, "%s %d +%s %s<BR>" , nchan->chan, nchan->users, w2c_GetCModes(nchan->modes), nchan->topic);
		}
	}
	return w2c_exit_client(cl, "OK");
}

int w2c_motd(WClient *cl, int parc, char **parv)
{
	char *name = parv[1], buf[500];
	FILE *fp = fopen(name, "r");

	if(!fp) w2c_sendrpl(cl, "Ce serveur n'existe pas ou MOTD non trouvé: %s", parv[1]);
	else {
		w2c_sendrpl(cl, "Message du jour de %s<BR>", name);
	 	while(fgets(buf, sizeof buf, fp)) w2c_sendrpl(cl, "%s<BR>", buf);
		w2c_sendrpl(cl, "Fin du Message du jour<BR>", name);
	}
	return w2c_exit_client(cl, "");
}

int w2c_checkuser(WClient *cl, int parc, char **parv)
{
	if(getuserinfo(parv[1])) w2c_sendrpl(cl, "1");
	else w2c_sendrpl(cl, "0");
	return w2c_exit_client(cl, "OK");
}
