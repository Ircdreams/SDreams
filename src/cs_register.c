/* src/cs_register.c - Enregistrement de salon
 * Copyright (C) 2004-2006 ircdreams.org
 *
 * contact: bugs@ircdreams.org
 * site web: http://www.ircdreams.org
 *
 * Services pour serveur IRC. Support? sur IrcDreams V.2
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
 * $Id: cs_register.c,v 1.35 2006/03/15 19:04:42 bugs Exp $
 */

#include <ctype.h>
#include "main.h"
#include "hash.h"
#include "add_info.h"
#include "fichiers.h"
#include "outils.h"
#include "cs_cmds.h"
#include "config.h"
#include "crypt.h"
#include "template.h"
#include "version.h"

/* regchan, unreg (+chancheck) + register, drop */

int chan_check(const char *chan, aNick *nick)
{
        aDNR *dnr;
        int i = 1;

        if(*chan != '#') return csreply(nick, GetReply(nick, L_CHANNELNAME));

        for(;chan[i] && i <= REGCHANLEN;++i)
                if(chan[i] == ',' || !isascii(chan[i])) /* bad char or comma */
                        return csreply(nick, GetReply(nick, L_USER_INVALID), chan);
        /* if chan[i] != \0 then i > REGCHANLEN */
        if(chan[i])     return csreply(nick, GetReply(nick, L_CHANMAXLEN), REGCHANLEN);

        if((dnr = IsBadChan(chan)))
        {
                if(IsAnAdmin(nick->user))
                        csreply(nick, "DNRMask: \2%s\2 Auteur: \2%s\2 Pos? le:\2 %s\2 Raison: %s",
                                dnr->mask, dnr->from, get_time(nick, dnr->date), dnr->raison);
                else return csreply(nick, GetReply(nick, L_CANREGDNR), chan, dnr->raison);
        }

        return 1;
}

/*
 * register_chan <salon> <th?me>
 */
int register_chan(aNick *nick, aChan *c, int parc, char **parv)
{
	char *salon = parv[1], theme[DESCRIPTIONLEN+1] = {0}, *ptr;
	aJoin *j;

	if(!chan_check(salon, nick)) return 0;

	if(CantRegChan(nick->user))
		return csreply(nick, "Vous ne pouvez pas enregistrer de salon%s%s.",
				(nick->user->cantregchan > 0) ? " jusqu'au " : "",
				(nick->user->cantregchan > 0) ? get_time(nick, nick->user->cantregchan) : "");

	parv2msgn(parc, parv, 2, theme, DESCRIPTIONLEN);

	if(strlen(theme) < 12)
		return csreply(nick, GetReply(nick, L_MINDESCRIPTIONLEN));

	if((ptr = IsAnOwner(nick->user)))
		return csreply(nick, GetReply(nick, L_ALREADYOWNER), ptr);

	if(getchaninfo(salon) || !strcasecmp(salon, bot.pchan)) return csreply(nick, GetReply(nick, L_ALREADYREG), salon);

	if(!(j = getjoininfo(nick, salon)) || !IsOp(j))
		return csreply(nick, GetReply(nick, L_NEEDTOBEOP), salon);

	c = add_chan(salon, theme);
	add_access(nick->user, salon, OWNERLEVEL, A_OP | A_PROTECT, CurrentTS);

	csjoin(c, JOIN_REG|JOIN_FORCE);
	cstopic(c, theme);
	
	putlog(LOG_CHANS, "REGCHAN %s par %s@%s (%s)", salon, nick->nick, nick->user->nick, theme);
	csreply(nick, GetReply(nick, L_CHANNOWREG), salon);

	return 1;
}

/*
 * ren_chan <chan> <nouveau chan>
 */
int ren_chan(aNick *nick, aChan *c, int parc, char **parv)
{
	aJoin *j;
	const char *newchan = parv[2];

	if(c->flag & C_ALREADYRENAME && !IsAdmin(nick->user))
		return csreply(nick, GetReply(nick, L_CMDALREADYUSED), parv[0]);

	if(!chan_check(newchan, nick)) return 0;

	if(getchaninfo(newchan) || !strcasecmp(newchan, bot.pchan))
		return csreply(nick, GetReply(nick, L_ALREADYREG), newchan);

	if(!IsAdmin(nick->user) && (!(j = getjoininfo(nick, newchan)) || !IsOp(j)))
		return csreply(nick, GetReply(nick, L_NEEDTOBEOP), newchan);

	cspart(c, newchan);

	putlog(LOG_CHANS, "RENCHAN %s %s par %s@%s", c->chan, newchan, nick->nick, nick->user->nick);

	if(CJoined(c)) cspart(c, newchan);

	/* csjoin will find the new NetChan */

	if(c->netchan) c->netchan->regchan = NULL, c->netchan = NULL;
	switch_chan(c, newchan);
	csjoin(c, JOIN_FORCE);

	c->flag |= C_ALREADYRENAME;

	csreply(nick, GetReply(nick, L_OKCHANGED));
	return 1;
}

/*
 * unreg_chan salon raison
 */
int unreg_chan(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	char tmp[300];

	if(!chaninfo->owner || chaninfo->owner->user != nick->user)
		return csreply(nick, GetReply(nick, L_NEEDTOBEOWNER));

	if(parc < 2 || strcasecmp(parv[2], "CONFIRME"))
		return csreply(nick, GetReply(nick, L_NEEDCONFIRM));

	csreply(nick, GetReply(nick, L_CHANUNREG), parv[1]);
	snprintf(tmp, sizeof tmp, "Unregister par %s@%s (%s)", nick->nick, nick->user->nick,
		parc > 2 ? parv2msg(parc, parv, 3, 150) : "Aucune raison");
	
	if(cf_unreg_reg_delay) nick->user->cantregchan = CurrentTS + cf_unreg_reg_delay;
	del_chan(chaninfo, 0, tmp);
	return 1;
}


/*
 * register <user> <mail> <mail> [pass]
 */
int register_user(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	anUser *user = NULL;
	char *p;
	aDNR *dnr;
	int flag = U_DEFAULT;

	if(nick->user && (GetConf(CF_ADMINREG) && !IsAdmin(nick->user))) 
		return csreply(nick, GetReply(nick, L_LOGUEDIN), nick->user->nick);

	if(GetConf(CF_ONLYWEBREG) && GetConf(CF_WEBSERV) && !GetConf(CF_PREMIERE) && !IsAnAdmin(nick->user))
		return csreply(nick, "La commande est d?sactiv?e, utilisez l'interface sur %s", webaddr); 

	if (GetConf(CF_ADMINREGONLY) && !GetConf(CF_PREMIERE) && !IsAnAdmin(nick->user))
		return csreply(nick, "Seul les Administrateurs peuvent enregistrer un compte.");

	/* on check ?a avant tout parce que ?a sert ? rien de faire les verifs sinon*/

	if(NRegister(nick) && !nick->user)
		return csreply(nick, GetReply(nick, L_CMDALREADYUSED), parv[0]);

	if(nick->user || GetConf(CF_PREMIERE|CF_NOMAIL))
	{
		if(parc < 4 || strlen(parv[4]) < 7) return csreply(nick, GetReply(nick, L_PASS_LIMIT));

		p = parv[4];
	}
	else p = create_password(nick->numeric), flag |= U_FIRST;

	if(strlen(parv[1]) > NICKLEN)
		return csreply(nick, GetReply(nick, L_USERLENLIMIT), NICKLEN);

	if(!IsValidNick(parv[1]))
		return csreply(nick, GetReply(nick, L_USER_INVALID), parv[1]);

	if((dnr = IsBadNick(parv[1])))
		return csreply(nick, GetReply(nick, L_CANREGDNR), parv[1], dnr->raison);
	/* le nick existe mais n'est pas le sien, risque d'abus*/
	if(!nick->user && strcasecmp(nick->nick, parv[1]) && getnickbynick(parv[1]))
		return csreply(nick, GetReply(nick, L_UCANTREG_INUSE));

	if (getuserinfo(parv[1]))
		return csreply(nick, GetReply(nick, L_ALREADYREG), parv[1]);

	if(strcasecmp(parv[3], parv[2]))
		return csreply(nick, "Les deux emails fournis sont diff?rents, vous avez surement commis une faute de frappe.");

	if(strlen(parv[2]) > MAILLEN)
		return csreply(nick, GetReply(nick, L_MAILLENLIMIT), MAILLEN);

	if(!IsValidMail(parv[2]))
		return csreply(nick, GetReply(nick, L_MAIL_INVALID));

	if(GetUserIbyMail(parv[2])) return csreply(nick, GetReply(nick, L_MAIL_INUSE));

	user = add_regnick(parv[1], MD5pass(p, NULL), CurrentTS, CurrentTS, 1, flag, parv[2], "none");
	user->lang = DefaultLang;
	
	if(nick->user)
		return csreply(nick, GetReply(nick, L_ADM_USER_REGUED), user->nick, p, user->mail);
	else if(GetConf(CF_PREMIERE|CF_NOMAIL))
	{
		SetNRegister(nick);
		cs_account(nick, user);
		if(GetConf(CF_PREMIERE))
		{
			user->level = MAXADMLVL;
			chaninfo = add_chan(bot.chan, "Salon d'aide ? propos des services SDreams");
			add_access(user, bot.chan, OWNERLEVEL, A_OP|A_PROTECT, CurrentTS);
			csjoin(chaninfo, JOIN_FORCE);

			csreply(nick, "Bienvenue sur les Services SDreams [" SPVERSION "] !");
			csreply(nick, "Vous ?tes Administrateurs des Services de niveau maximum.");
			csreply(nick, "Le salon %s vient d'?tre enregistr? sous votre Username", bot.chan);
                        db_write_users();
                        db_write_chans();
			ConfFlag &= ~CF_PREMIERE;
		}
		else csreply(nick, GetReply(nick, L_ADM_USER_REGUED), parv[1], p, parv[2]);
		csreply(nick, "\2Vous avez ?t? identifi? ? TOUS les services");
		csreply(nick, "Lors de votre prochaine connexion vous pourrez vous identifier avec"
			" : \2/%s %s %s %s", cs.nick, RealCmd("login"), parv[1], p);
		csreply(nick, "Le mail (%s) que vous avez fourni peut ?tre utilis? pour"
			" vous envoyer un pass en cas de probl?me.", parv[2]);
		csreply(nick, "Vous pouvez voir la liste des commandes avec \2/%s %s\2",
			cs.nick, RealCmd("showcommands"));
	}
	else
	{
		if(!tmpl_mailsend(&tmpl_mail_register, user->mail, user->nick, p, nick->host, NULL, NULL))
			return csreply(nick, "Impossible d'envoyer le mail, votre pass est %s", p);
		
		csreply(nick, GetReply(nick, L_PASS_SENT), user->mail);
		csreply(nick, GetReply(nick, L_REGISTERTIMEOUT), duration(cf_register_timeout));
	}
	return 1;
}

int drop_user(aNick *nick, aChan *chaninfo, int parc, char **parv)
{
	anUser *u = nick->user;
	char tmp[80] = {0};

	if(parc < 2 || strcasecmp(parv[2], "confirme"))
		return csreply(nick, GetReply(nick, L_NEEDCONFIRM));

	if(!checkpass(parv[1], u))
		return csreply(nick, GetReply(nick, L_BADPASS), u->nick);

	csreply(nick, GetReply(nick, L_OKDELETED), u->nick);

	if(IsAnOwner(u))
		snprintf(tmp, sizeof tmp, "Drop de l'username de l'owner (\2%s\2)", u->nick);

	cs_account(nick, NULL);
	del_regnick(u, 0, tmp);
	return 1;
}
