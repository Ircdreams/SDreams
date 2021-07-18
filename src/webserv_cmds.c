 /* src/webserv_cmds.c - commandes du Webserv
 * Copyright (C) 2004-2005 ircdreams.org
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
 * $Id: webserv_cmds.c,v 1.7 2006/03/23 18:55:03 bugs Exp $
 */

#include "main.h"
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/socket.h>
#include <errno.h>
#include "add_info.h"
#include "admin_user.h"
#include "config.h"
#include "crypt.h"
#include "cs_cmds.h"
#include "del_info.h"
#include "divers.h"
#include "fichiers.h"
#include "hash.h"
#include "memoserv.h"
#include "outils.h"
#include "vote.h"
#include "template.h"
#include "webserv.h"
#include "welcome.h"

/* **** CMDS **** */

int w2c_passwd(WClient *cl, int parc, char **parv)
{
	if(IsAuth(cl)) return 0; /* On devrait pas kill le sock?*/
	if(strcmp(parv[1], bot.w2c_pass))
		return w2c_exit_client(cl, "erreur Mauvais pass Webserv");
	cl->flag |= W2C_AUTH;
	return 0;
}

int w2c_login(WClient *cl, int parc, char **parv)
{
	anUser *user;
	time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);

	if(cl->user) return w2c_exit_client(cl, "erreur Déjà logué");

	if(!(user = getuserinfo(parv[1])))
		return w2c_exit_client(cl, "erreur Username inexistant");

	if(IsSuspend(user)) return w2c_exit_client(cl, "erreur Username suspendu");

	if(strcmp(user->passwd, parv[2]))	
	{
		if(IsAdmin(user))
		{
			cswall("(Web) Login Admin échoué sur \2%s\2 par \2%s\2", user->nick, parv[3]);
			putlog(LOG_FAUTH, "LOGIN %s (BadPass) par %s (Web)", user->nick, parv[3]);
		}
		return w2c_exit_client(cl, "erreur Mauvais pass");
	}

	user->lastseen = CurrentTS; /* login stuff : lastseen, lastlogin*/

	if(!user->lastlogin || strcmp(parv[3], user->lastlogin + 4))
	{
		char host[20] = "web@"; /* IPv4 + 'web@' + \0 */
		Strncpy(host + 4, parv[3], sizeof host - 5); /* wanna overflow me? */

		if(UOubli(user)) DelUOubli(user);
		str_dup(&user->lastlogin, host);
		putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] \2\0033LOGIN\2\3 %s par %s", cs.num, 
				bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec,
				user->nick, host);
	}
	cl->user = user;
	return 0;
}

int w2c_oubli(WClient *cl, int parc, char **parv) 
{ 
        anUser *user;
	char *passwd;
 
        if(!(user = getuserinfo(parv[1]))) 
                return w2c_exit_client(cl, "erreur Username inexistant"); 
 
        if(cl->user || user->n) return w2c_exit_client(cl, "erreur Déjà logué"); 
 
        if(IsSuspend(user)) return w2c_exit_client(cl, "erreur Username suspendu"); 
 
        if(UOubli(user)) return w2c_exit_client(cl, "erreur Pass déjà demandé"); 
 
        if(strcasecmp(user->mail, parv[2])) 
                return w2c_exit_client(cl, "erreur Mail ne concorde pas."); 
   
        SetUOubli(user);
	passwd = create_password(user->passwd); 
        w2c_sendrpl(cl, "newpass %s", passwd); 
        return w2c_exit_client(cl, "OK"); 
} 


#define SHOW_TOPIC 		0x01
#define SHOW_RIGHT 		0x02
#define SHOW_USERS 		0x04
#define SHOW_DEFMODES	 	0x08
#define SHOW_OWNER 		0x10
#define SHOW_BANS 		0x20
#define SHOW_ACCESS 		0x40
#define SHOW_ALL 		0x80

static int w2c_showaccess(WClient *cl, aChan *chan, int flag)
{
	aLink *lp = chan->access;
	int count = 0;

	for(;lp;lp = lp->next)
	{
		anAccess *a = lp->value.a;
		w2c_sendrpl(cl, "access%d %s %d %lu %d :%s", ++count, a->user->nick, a->level,
			a->lastseen, a->flag, NONE(a->info));
	}

	return w2c_sendrpl(cl, "accesscount %d", count);
}

int w2c_channel(WClient *cl, int parc, char **parv)
{
        aChan *chan = getchaninfo(parv[1]);
        int i=1;
        anUser *user;
        anAccess *a;
	aLink *lp;
        char *salon = parv[1];

	if(!strcasecmp(parv[2], "info"))
	{
		int infos = 0;
		aNChan *netchan;

		if(!chan) return w2c_exit_client(cl, "erreur Salon non reg");
		else if(parc < 4) return w2c_exit_client(cl, "OK Salon reg");

		if(IsAdmin(cl->user) || GetAccessIbyUserI(cl->user, chan)) infos |= SHOW_RIGHT;
		if((netchan = chan->netchan) && HasMode(netchan, C_MSECRET|C_MPRIVATE) && !(infos & SHOW_RIGHT))
			return w2c_exit_client(cl, "erreur Ces infos ne vous sont disponibles.");

		for(i = 3;i < parc;++i)
		{
			if(!strcasecmp(parv[i], "all"))
			{
				infos |= SHOW_ALL;
				break;
			}
			else if(!strcasecmp(parv[i], "topic")) infos |= SHOW_TOPIC;
			else if(!strcasecmp(parv[i], "owner")) infos |= SHOW_OWNER;
			else if(!strcasecmp(parv[i], "users") && infos&SHOW_RIGHT) infos |= SHOW_USERS;
			else if(!strcasecmp(parv[i], "bans")) infos |= SHOW_BANS;
			else if(!strcasecmp(parv[i], "access")) infos |= SHOW_ACCESS;
		}

		if(!infos) return w2c_exit_client(cl, "OK Salon reg");/* aucune infos demandées*/

		if(infos & SHOW_OWNER)/* OK, construisons/envoyons les infos*/
			w2c_sendrpl(cl, "owner %s", chan->owner ? chan->owner->user->nick : "<Aucun>");
		if(infos & SHOW_ALL)
		{
			w2c_sendrpl(cl, "=owner=%s ct=%lu banlevel=%d cml=%d bantype=%d "
				"bantime=%ld options=%d defmodes=%u deflimit=%d defkey=%s",
				chan->owner ? chan->owner->user->nick : "***", chan->creation_time,
				chan->banlevel,	chan->cml, chan->bantype, chan->bantime, chan->flag,
				chan->defmodes.modes, chan->defmodes.limit,
				!HasDMode(chan, C_MKEY) || infos & SHOW_RIGHT ? chan->defmodes.key : "***");

			if(netchan) w2c_sendrpl(cl, "=modes=%u limit=%d key=%s",
				netchan->modes.modes, netchan->modes.limit,
				!HasMode(netchan, C_MKEY) || infos & SHOW_RIGHT ? netchan->modes.key : "***");
			else w2c_sendrpl(cl, "=modes=0 limit=0 key=");

			w2c_sendrpl(cl, "description %s", chan->description);
			if(*chan->deftopic) w2c_sendrpl(cl, "deftopic %s", chan->deftopic);
			if(*chan->welcome) w2c_sendrpl(cl, "welcome %s", chan->welcome);
			if(*chan->url) w2c_sendrpl(cl, "chanurl %s", chan->url);
			if(infos & SHOW_RIGHT && chan->motd) w2c_sendrpl(cl, "motd %s", chan->motd);
		}
		if(infos & (SHOW_ALL|SHOW_TOPIC) && netchan && *netchan->topic)
			w2c_sendrpl(cl, "topic %s", netchan->topic);
		if(infos & SHOW_ACCESS) w2c_showaccess(cl, chan, 0);
		if(infos & SHOW_BANS)
		{
			aBan *ban = chan->banhead;

			for(i = 0;ban;ban = ban->next)
				w2c_sendrpl(cl, "ban%u %s %s %lu %lu %d :%s", ++i, ban->mask,
					ban->de, ban->debut, ban->fin, ban->level, ban->raison);
			w2c_sendrpl(cl, "bancount %u", i);
		}
	}
	else if(!strcasecmp(parv[2], "set")) 
        {
		int level = 500, itype = 0, changes = 0;

                if(!chan) return w2c_exit_client(cl, "erreur Salon non reg"); 
                if(!IsAdmin(cl->user) && (level = ChanLevelbyUserI(cl->user, chan)) < 450) 
                        return w2c_exit_client(cl, "erreur Accès refusé");

                for(i = 4;i < parc; i += 2)
		{
			const char *opt = parv[i - 1], *param = parv[i];
			
                        if(!strcasecmp(opt, "deftopic")) {
                                w2c_parse_qstring(chan->deftopic, sizeof chan->deftopic, parv, parc, i, &i);
				if(CJoined(chan)) cstopic(chan, chan->deftopic);
			}
                        else if(!strcasecmp(opt, "theme")) 
                                w2c_parse_qstring(chan->description, sizeof chan->description, parv, parc, i, &i); 
                        else if(!strcasecmp(opt, "welcome")) 
                                w2c_parse_qstring(chan->welcome, sizeof chan->welcome, parv, parc, i, &i); 
                        else if(!strcasecmp(opt, "chanurl")) 
                                w2c_parse_qstring(chan->url, sizeof chan->url, parv, parc, i, &i); 
                        else if(!strcasecmp(opt, "motd")) 
                        { 
                                char buf[TOPICLEN + 1]; 
                                w2c_parse_qstring(buf, sizeof buf, parv, parc, i, &i); 
                                if(*buf) str_dup(&chan->motd, buf); 
                                else free(chan->motd), chan->motd = NULL; 
                        } 
                        else if(!strcasecmp(opt, "defkey"))
                                Strncpy(chan->defmodes.key, param, KEYLEN);
                        else if(!strcasecmp(opt, "deflimit") && (itype = strtol(param, NULL, 10)) >= 0)
                                chan->defmodes.limit = itype;
                        else if(!strcasecmp(opt, "defmodes")) {
				if(CJoined(chan)) csmode(chan, 0, "-$", GetCModes(chan->netchan->modes));
				chan->defmodes.modes = strtol(param, NULL, 10);
				if(CJoined(chan)) csmode(chan, 0, "+$", GetCModes(chan->defmodes));
			}
                        else if(!strcasecmp(opt, "banlevel") && level >= chan->banlevel
				&& (itype = strtol(param, NULL, 10)) >= 0 && itype <= level)
                                                chan->banlevel = itype; 
                        else if(!strcasecmp(opt, "cml") && level >= chan->cml
				&& (itype = strtol(param, NULL, 10)) >= 0 && itype <= level)
                                                chan->cml = itype; 
                        else if(!strcasecmp(opt, "bantime") && (itype = strtol(param, NULL, 10)) >= 0)
                                chan->bantime = itype;
                        else if(!strcasecmp(opt, "flags") && (itype = strtol(param, NULL, 10)) >= 0) {
                                chan->flag = itype; 
				if(CFLimit(chan)) {
					if(!chan->limit_inc || !chan->limit_min) {
                                		chan->limit_inc = DEFAUT_LIMITINC;
                                		chan->limit_min = DEFAUT_LIMITMIN;
					}
					if(CJoined(chan)) floating_limit_update_timer(chan);
                        	}
			}
                        else if(!strcasecmp(opt, "bantype") && (itype = strtol(param, NULL, 10)) > 0 && itype <= 5)
                                chan->bantype = itype;
                        else continue;
			++changes;
                }
		if(!changes) return w2c_exit_client(cl, "erreur Syntaxe incorrecte");
	}
	else if(!strcasecmp(parv[2], "rename"))
	{
		char *newchan = parv[3];
		int k=0, level = 500;

		if(!IsAdmin(cl->user) && (level = ChanLevelbyUserI(cl->user, chan)) < 500)
                        return w2c_exit_client(cl, "erreur Accès refusé");

		for(;newchan[k] && k <= REGCHANLEN && newchan[k] != ',' && isascii(newchan[k]);++k);
		if(newchan[k] || *newchan != '#') return w2c_exit_client(cl, "erreur Syntaxe incorrecte");
 
 		if(chan->flag & C_ALREADYRENAME && !IsAdmin(cl->user))
 			return w2c_exit_client(cl, "erreur Salon déjà renomé");
 
 		if(getchaninfo(newchan) || !strcasecmp(newchan,bot.pchan))
 			return w2c_exit_client(cl, "erreur Salon déjà enregistré");
 
		if(GetNChan(newchan)) return w2c_exit_client(cl, "erreur Salon non vide");		
		if(CJoined(chan)) cspart(chan, newchan);
 
		chan->netchan = NULL; /* csjoin will find the new NetChan */
		switch_chan(chan, newchan);
		csjoin(chan, JOIN_FORCE);
		chan->flag |= C_ALREADYRENAME;
	}
        else if(!strcasecmp(parv[2], "adduser"))
        {
                int lvl,level = 450, ma;

                if(!IsAdmin(cl->user) && (level = ChanLevelbyUserI(cl->user, chan)) < 450)
                        return w2c_exit_client(cl, "erreur Accès refusé");

	        for(ma = 0, lp = chan->access;lp;lp = lp->next, ma++)
        	        if(ma >= WARNACCESS - 1) return w2c_exit_client(cl, "erreur La liste des accès est pleine");

                if(!(user = getuserinfo(parv[3]))) return  w2c_exit_client(cl, "erreur Username inexistant");

                if(!is_num(parv[4]) || (lvl = atoi(parv[4])) < 1 || lvl >= OWNERLEVEL)
                        return w2c_exit_client(cl, "erreur Level non valide");

                if(lvl >= ChanLevelbyUserI(cl->user, chan) && !IsAdmin(cl->user))
                        return w2c_exit_client(cl, "erreur Level superieur ou égal au votre");

                if(UPReject(user))
                        return  w2c_exit_client(cl, "erreur Username n'accepte pas les accès");

                for(a = user->accesshead;a && a->c != chan;a = a->next);
                if(a)
                {
                        if(AWait(a)) return  w2c_exit_client(cl, "erreur Accès en attente");
                        else  w2c_exit_client(cl, "erreur Déjà Accès");
                }

                if(UPAccept(user))
                {
                        add_access(user, salon, lvl, (lvl >= 400) ? (A_OP | A_PROTECT) : 0, CurrentTS);

                        if(user->n) csreply(user->n, "%s vous a donné un accès de niveau %d sur %s.", cl->user, lvl, salon);
                }
                else
                {
                        add_access(user, salon, lvl, lvl >= 400 ? A_OP|A_PROTECT|A_WAITACCESS : A_WAITACCESS, CurrentTS);
                        if(user->n)
                        {
                                csreply(user->n, "%s vous a proposé un access de %d sur %s", cl->user, lvl, salon);
                                csreply(user->n, "Pour accepter tapez: \2/%s %s ACCEPT %s\2, pour refuser tapez: \2/%s %s REFUSE %s\2", cs.nick, 
					RealCmd("myaccess"), salon, cs.nick, RealCmd("myaccess"), salon);
                        }
                }
		if(GetConf(CF_MEMOSERV))
        	{
                	char memo[MEMOLEN + 1], memomail[MEMOLEN +1];
                	if(UPAsk(user))
                        	snprintf(memomail, MEMOLEN, "%s vous a proposé un access de %d sur %s.\nPour accepter tapez: /%s %s ACCEPT %s\nPour refuser tapez: /%s %s REFUSE %s",
                                	cl->user->nick, lvl, salon, cs.nick, RealCmd("myaccess"), salon, cs.nick, RealCmd("myaccess"), salon);
                	else
                        	snprintf(memomail, MEMOLEN, "\2%s\2 vous a donné un accès de niveau %d sur \2%s\2.", cl->user->nick, lvl, salon);

                	snprintf(memo, MEMOLEN, UPAsk(user) ? "%s vous a proposé un access de %d sur %s." : "\2%s\2 vous a donné un accès de niveau %d sur \2%s\2.",
                        	cl->user->nick, lvl, salon);

                	if(!UNoMail(user)) tmpl_mailsend(&tmpl_mail_memo, user->mail, user->nick, NULL, NULL, cl->user->nick, memomail);
                	if(!user->n) add_memo(user, cl->user->nick, CurrentTS, memo, MEMO_AUTOEXPIRE);
        	}
        }
        else if(!strcasecmp(parv[2], "deluser"))
        {
                int level = 450;
                if(!IsAdmin(cl->user) && (level = ChanLevelbyUserI(cl->user, chan)) < 450)
                        return w2c_exit_client(cl, "erreur Accès refusé");

                if(!(user = getuserinfo(parv[3]))) return w2c_exit_client(cl, "erreur Username inexistant");

                for(a = user->accesshead;a && a->c != chan;a = a->next);
                if(!a) return w2c_exit_client(cl, "erreur Username Pas Accès");

                if(ChanLevelbyUserI(cl->user, chan) <= a->level && !IsAdmin(cl->user))
                        return w2c_exit_client(cl, "erreur  Level superieur ou égal au votre");

                if(a->level == OWNERLEVEL) return w2c_exit_client(cl, "erreur Username est l'owner");

		if(GetConf(CF_MEMOSERV))
                {
                        char memomail[MEMOLEN +1];
                        snprintf(memomail, MEMOLEN, "%s vous supprimé votre accès sur %s", cl->user->nick, chan->chan);

			if(!UNoMail(user)) tmpl_mailsend(&tmpl_mail_memo, user->mail, user->nick, NULL, NULL, cl->user->nick, memomail);
                        if(!user->n) add_memo(user, cl->user->nick, CurrentTS, memomail, MEMO_AUTOEXPIRE);
                }
		if(user->n) csreply(user->n, "%s a vous supprimé votre accès sur %s", cl->user->nick, chan->chan);

                del_access(user, chan);
        }
	else return w2c_exit_client(cl, "erreur Syntaxe incorrecte");
	return w2c_exit_client(cl, "OK");
}

static int w2c_memolist(WClient *cl, anUser *user, int id)
{
	aMemo *m = user->memohead;
	int i = 0;

	for(;m;m = m->next)
	{
		if(++i == id) return w2c_sendrpl(cl, "memo %s %lu %d :%s",
					m->de, m->date, m->flag, m->message);
		else if(!id) w2c_sendrpl(cl, "memo%d %s %lu %d :%s", i,
					m->de, m->date, m->flag, m->message), m->flag |= MEMO_READ;
	}
	w2c_sendrpl(cl, "memocount %d", i); /* only sent if id == 0 */
	return 0;
}

int w2c_user(WClient *cl, int parc, char **parv)
{
	anUser *user;
	anAlias *alias = NULL;
	int adm = IsAdmin(cl->user) ? cl->user->level : 0;

	/* handle most of case whithout looping: Not Admin or query on himself */
	if(!adm || !strcasecmp(parv[1], cl->user->nick)) user = cl->user;
	else if(!(user = getuserinfo(parv[1]))) /* check admin query */
		return w2c_exit_client(cl, "erreur Username inconnu");

	if(!strcasecmp(parv[2], "info"))
	{
		anAccess *a;
		int countaxx = 0, countalias = 0;

		w2c_sendrpl(cl, "=user=%s mail=%s lastseen=%lu level=%d lastlogin=%s "
			"options=%d regtime=%ld vhost=%s lang=%s", user->nick, user->mail, user->lastseen,
			user->level, user->lastlogin ? user->lastlogin : "<Indisponible>",
			user->flag, user->reg_time, user->vhost, user->lang->langue);

		if(user->swhois) w2c_sendrpl(cl, "swhois %s", user->swhois);

		for(alias = user->aliashead;alias;alias = alias->user_nextalias)
                        w2c_sendrpl(cl, "alias%d %s", ++countalias, alias->name);
		w2c_sendrpl(cl, "aliascount %d", countalias);

		if(user == cl->user) w2c_memolist(cl, user, 0);

		for(a = user->accesshead;a;a = a->next)
			w2c_sendrpl(cl, "access%d %s %d %d %lu :%s", ++countaxx, a->c->chan,
				a->level, a->flag, a->lastseen, NONE(a->info));
		w2c_sendrpl(cl, "accesscount %d", countaxx);
	}
	else if(!strcasecmp(parv[2], "set"))
	{
		/*
		 * Set: can handle multi-options change on single line
		 */
		int i = 4, changes = 0, count = 0;

		if(adm && user->level >= adm && user != cl->user)
			return w2c_exit_client(cl, "erreur Accès refusé");

		for(;i < parc; i += 2) {
			char *opt = parv[i-1], *param = parv[i];

			if(!strcasecmp(opt, "mail")) {
				if(strlen(param) > MAILLEN || !IsValidMail(param) || !strcasecmp(user->mail, param))
					return w2c_exit_client(cl, "erreur invalid arg");

				if(GetUserIbyMail(param)) return w2c_exit_client(cl, "erreur mail déjà utilisé");

				switch_mail(user, param);
				++changes;
			} /* mail */

			else if(!strcasecmp(opt, "addalias")) {
				if(strlen(param) > NICKLEN || !IsValidNick(param))
                                        return w2c_exit_client(cl, "erreur invalid arg");

                                if(IsBadNick(param))
					return w2c_exit_client(cl, "erreur Alias interdit");

				for(alias = user->aliashead;alias;alias = alias->user_nextalias) ++count;

				if(count >= MAXALIAS && !IsAdmin(user))
					return w2c_exit_client(cl, "erreur Quota d'alias atteint");

                                if(getuserinfo(param))
					return w2c_exit_client(cl, "erreur Alias déjà utilisé");
				add_alias(user, param);
				++changes;
			}

			else if(!strcasecmp(opt, "delalias")) {
				if(!checknickaliasbyuser(param,user)) return w2c_exit_client(cl, "erreur Alias inconnu");
				del_alias(user, param);
				++changes;
			}

			else if(!strcasecmp(opt, "pass")) {
				if(strlen(param) < 7) return w2c_exit_client(cl, "erreur invalid arg");

				MD5pass(param, user->passwd);
                                SetUMD5(user); /* mark it as md5 */
				++changes;
			} /* password */

			else if(!strcasecmp(opt, "username")) {
				if(!GetConf(CF_USERNAME))
					return w2c_exit_client(cl, "erreur commande désactivée");
				if(user->flag & U_ALREADYCHANGE && !adm)
					return w2c_exit_client(cl, "erreur username déjà changé");

				if(strlen(param) > NICKLEN || !IsValidNick(param))
					return w2c_exit_client(cl, "erreur invalid arg");

				if(IsBadNick(param)) return w2c_exit_client(cl, "erreur User interdit");

				if(getuserinfo(param)) return w2c_exit_client(cl, "erreur user déjà utilisé");

				switch_user(user, param);
				++changes;
				if(!adm) user->flag |= U_ALREADYCHANGE;
			} /* username */

			else if(!strcasecmp(opt, "lang")) {
				Lang *lang = lang_isloaded(param);

				if(!lang)
					return w2c_exit_client(cl, "erreur language non disponible");

				user->lang = lang;
			} /* lang */

			else if(!strcasecmp(opt, "flag")) {
				int flag = atoi(param);

				if(!flag || flag == user->flag) continue; /* not a number, or no change */

#define U_ACCESSPOLICY (U_PREJECT|U_POKACCESS|U_PACCEPT)

				flag &= U_ALL; /* clear stranger flags */
				++changes;

				if(user->flag & U_ALREADYCHANGE && !(flag & U_ALREADYCHANGE))
					flag |= U_ALREADYCHANGE;

		/*		if((U_PKILL|U_PNICK) & flag == U_PKILL|U_PNICK
					|| !(U_ACCESSPOLICY & flag)	|| U_ACCESSPOLICY & ~(U_ACCESSPOLICY & flag))
					return w2c_exit_client(cl, "erreur flag nécessaire manquant");*/

				if((U_ACCESSPOLICY & user->flag) != (U_ACCESSPOLICY & flag))
				{
					anAccess *a, *at;
					if(flag & U_PREJECT) /* refuse all pending propositions */
						for(a = user->accesshead;a;a = at) {
							at = a->next;
							if(AWait(a)) del_access(user, a->c);
						}
					else if(flag & U_PACCEPT) /* accept all propositions */
						for(a = user->accesshead;a;a = a->next)
							if(AWait(a)) a->flag &= ~A_WAITACCESS;
				}
				user->flag = flag;
			} /* options */

		} /* for */

		/* no change performed */
		if(!changes) return w2c_exit_client(cl, "erreur Syntaxe incorrecte");

		/* at least one (doesn't check for bad syntax in extra changes) */
		w2c_sendrpl(cl, "=user=%s mail=%s level=%d options=%d lang=%s",
			user->nick, user->mail, user->level, user->flag, user->lang->langue);
	} /* set */
	else if(!strcasecmp(parv[2], "aaccept") || !strcasecmp(parv[2], "adrop"))
	{
		anAccess *a;
		aChan *c;
		int changes = 0, j = 2, del = 0;

		for(;j < parc;++j) {
			if(!strcasecmp(parv[j], "adrop")) del = 1;
			else if(!strcasecmp(parv[j], "aaccept")) del = 0;
			else for(a = user->accesshead, c = getchaninfo(parv[j]); c && a;a = a->next)
				if(a->c == c)
				{
					if(del) del_access(user, a->c), ++changes;
					else if(AWait(a)) a->flag &= ~A_WAITACCESS, ++changes;
					break;
				}
		}

		if(!changes) return w2c_exit_client(cl, "erreur Syntaxe incorrecte");
		w2c_sendrpl(cl, "performed %d", changes);
	} /* aaccess */

	return w2c_exit_client(cl, "OK");
}

int w2c_register(WClient *cl, int parc, char **parv)
{
	anUser *user;
	aNick *nick;
	time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);

	if(cl->user) return w2c_exit_client(cl, "erreur Vous possédez déjà un username");

	if(strlen(parv[1]) > NICKLEN || strlen(parv[2]) > MAILLEN
		|| !IsValidNick(parv[1]) || !IsValidMail(parv[2]) || strlen(parv[3]) < 6)
		return w2c_exit_client(cl, "erreur arg invalide");

	if(IsBadNick(parv[1])) return w2c_exit_client(cl, "erreur username non enregistrable");
	if(getuserinfo(parv[1])) return w2c_exit_client(cl, "erreur user déjà enregistré");

	if((nick = getnickbynick(parv[1])) && strcmp(GetIP(nick->base64), parv[4]))
		return w2c_exit_client(cl, "erreur ce pseudo est utilisé par quelqu'un d'autre");

	if(GetUserIbyMail(parv[2])) return w2c_exit_client(cl, "erreur mail déjà utilisé");

	 user = add_regnick(parv[1], MD5pass(parv[3], NULL), CurrentTS, CurrentTS, 
                                   1, U_DEFAULT|U_FIRST, parv[2], "none");

	cl->user = user;
	user->lang = DefaultLang;

	putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] \2\0033REGISTER\2\3 %s par web@%s", cs.num,
                                bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec,
                                user->nick, parv[4]);

	return w2c_exit_client(cl, "OK");
}

int w2c_regchan(WClient *cl, int parc, char **parv)
{
	time_t tmt = CurrentTS;
        struct tm *ntime = localtime(&tmt);
	aChan *chan;
	const char *c = parv[1];
	int i = 1;

	for(;c[i] && i < REGCHANLEN && c[i] != ',' && isascii(c[i]);++i);

	if(c[i] || *c != '#') return w2c_exit_client(cl, "erreur Syntaxe incorrecte");

	if (!parv[2]) return w2c_exit_client(cl, "erreur Description manquante");

	if(getchaninfo(c)) return w2c_exit_client(cl, "erreur AlreadyExist");
	if(IsBadChan(c)) return w2c_exit_client(cl, "erreur Salon interdit");

	if(CantRegChan(cl->user)) return w2c_exit_client(cl, "erreur CantRegChan");
	if(IsAnOwner(cl->user)) return w2c_exit_client(cl, "erreur AlreadyOwner");
	if(GetNChan(c)) return w2c_exit_client(cl, "erreur ChannelNotEmpty");

	chan = add_chan(c, parv2msg(parc-1, parv, 2, DESCRIPTIONLEN));
	add_access(cl->user, c, OWNERLEVEL, A_OP|A_PROTECT, CurrentTS);

	putserv("%s " TOKEN_PRIVMSG " %s :[%02d:%02d:%02d] \2\0033REGCHAN\2\3 %s par %s (%s)", cs.num,
                                bot.pchan, ntime->tm_hour, ntime->tm_min, ntime->tm_sec,
                                c, cl->user->nick, chan->description);

	return w2c_exit_client(cl, "OK");
}

int w2c_memo(WClient *cl, int parc, char **parv)
{
	aMemo *memo = cl->user->memohead;
	const char *cmd = parv[1];
	int i = 0, j = 0;

	if(!strcasecmp(cmd, "READ")) w2c_memolist(cl, cl->user, 0);
	else if(!strcasecmp(cmd, "DEL"))
	{
		int todel[10], i = 0;

		if(parc < 2 || (strcasecmp(parv[2], "all")
			&& !(i = item_parselist(parv[2], todel, ASIZE(todel)))))
			return w2c_exit_client(cl, "erreur arg invalide");

		memo_del(cl->user, todel, i);

		w2c_memolist(cl, cl->user, 0);
	}
	else if(!strcasecmp(cmd, "SEND"))
	{
		anUser *user;
		aNick *who;
		char *tmp;
	
		if(parc < 3) return w2c_exit_client(cl, "erreur erreur de syntaxe");

		if(!(user = getuserinfo(parv[2])))
			return w2c_exit_client(cl, "erreur Username inconnu");

		if(user == cl->user)
			return w2c_exit_client(cl, "erreur Vous ne pouvez vous envoyer de memo a vous meme");
		
		if(UNoMemo(user) && !IsAdmin(cl->user))
			return w2c_exit_client(cl, "erreur Il n'accepte pas de memo");

		tmp = parv2msg(parc-1, parv, 3, MEMOLEN);
			
		if((who = user->n) && !IsAway(who))
			csreply(who, "\2Mémo de %s@WebServ\2 %s:", cl->user->nick, tmp);
	
		for(memo = user->memohead;memo;memo = memo->next)
			if(!strcasecmp(memo->de, cl->user->nick)) j++;

		if(i >= MAXMEMOS && !IsAdmin(cl->user))
			return w2c_exit_client(cl, "erreur Trop de Memos envoyé à cette personne");

		add_memo(user, cl->user->nick, CurrentTS, tmp, 0);
	}
	return w2c_exit_client(cl, "OK");
}

int w2c_restart(WClient *cl, int parc, char **parv)
{
        FILE *fuptime;
        char *r = parv2msg(parc-1, parv, 1, 300);

        if (cl->user->level != MAXADMLVL) return w2c_exit_client(cl, "erreur Non autorisé");

        if((fuptime = fopen("uptime.tmp", "w"))) /* permet de maintenir l'uptime (/me tricheur)*/
        {
                fprintf(fuptime, "%lu", bot.uptime);
                fclose(fuptime);
        }

        putserv("%s " TOKEN_QUIT " :%s [\2Redémarrage\2]", cs.num, r);
        putserv("%s " TOKEN_SQUIT " %s 0 :%s [\2Redémarrage\2]", bot.servnum, bot.server, r);

#ifdef USEBSD
	usleep(500000);
#endif
        ConfFlag |= CF_RESTART;
	running = 0;

        return w2c_exit_client(cl, "OK");
}

int w2c_write_files(WClient *cl, int parc, char **parv)
{
	int write = 0;

	if (!IsAdmin(cl->user)) return w2c_exit_client(cl, "erreur Non autorisé");

	if(getoption("user", parv, parc-1, 1, -1)) write |= 0x1;
        if(getoption("chan", parv, parc-1, 1, -1)) write |= 0x2;
	if(getoption("all", parv, parc-1, 1, -1)) write |= 0;

        switch(write)
        {
		case 0:
                        if(GetConf(CF_WELCOMESERV)) write_welcome();
                        write_dnr();
                        write_cmds();
                        if(GetConf(CF_VOTESERV)) write_votes();
                        write_maxuser();
			db_write_chans();
                        db_write_users();
                        cswallops("Ecriture des bases de données");
                        break;
                case 0x1|0x2:
                        db_write_chans();
                        db_write_users();
                        cswallops("Ecriture des bases de données users/chans");
                        break;
                case 0x2:
                        db_write_chans();
                        cswallops("Ecriture de la base donnée chans");
                        break;
                case 0x1:
                        db_write_users();
                        cswallops("Ecriture de la base donnée users");
                        break;
        }
	return w2c_exit_client(cl, "OK");
}
