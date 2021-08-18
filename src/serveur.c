/* src/serveur.c - Traitement des messages IRC
 *
 * Copyright (C) 2002-2008 David Cortier  <Cesar@ircube.org>
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
 * $Id: serveur.c,v 1.247 2008/01/05 18:34:14 romexzf Exp $
 */

#include "main.h"
#include "outils.h"
#include "serveur.h"
#include "add_info.h"
#include "admin_manage.h"
#include "hash.h"
#include "flood.h"
#include "cs_cmds.h"
#include "config.h"
#include "del_info.h"
#include "debug.h"
#include "mylog.h"
#include "aide.h"
#include "timers.h"
#include <sys/time.h>
#ifdef USE_MEMOSERV
#include "memoserv.h"
#endif
#ifdef USE_WELCOMESERV
#include "welcome.h"
#endif
#ifdef HAVE_TRACK
#include "track.h"
#endif
#include <ctype.h>

aServer *mainhub = NULL;
static int burst = 0;

#ifdef HAVE_OPLEVELS
/*
 * m_destruct SS DE # timestamp
 */
int m_destruct(int parc, char **parv)
{
	aNChan *c;

	if(parc < 3) return Debug(W_DESYNCH|W_WARN, "DE: pas assez d'argument : %d args", parc);

	if(!(c = GetNChan(parv[1])) || strtol(parv[2], NULL, 10) > c->timestamp
		|| c->users || (c->regchan && CJoined(c->regchan)))
		return 0; /* Salon non trouvé, on l'a peut être déjà supprimé ? */

	del_nchan(c); /* It's really a zannel */
	return 0;
}
#endif

/*
 * m_clearmode SSCCC CM # mode_cleared
 */
int m_clearmode(int parc, char **parv)
{
	aNChan *chan = GetNChan(parv[1]);
	aLink *j;
	int clear = 0;
	char mode[30] = "-";

	if(!chan) return Debug(W_DESYNCH|W_WARN, "CM %s de %s: aNChan non trouvé!", parv[1], parv[0]);

	Strncpy(mode + 1, parv[2], sizeof mode -2);
	/* string2scmode n'enleve le +k que si une key est fournie, ce *hack* joue bien le jeu */
	string2scmode(&chan->modes, mode, strchr(parv[2], 'k'), NULL);

	if(strchr(parv[2], 'o')) clear |= J_OP;
	if(strchr(parv[2], 'v')) clear |= J_VOICE;

	if(clear)  /* changement de +o|v */
		for(j = chan->members; j; j = j->next)
			 if(!(j->value.j->nick->flag & (N_SERVICE | N_GOD))) j->value.j->status &= ~clear;
	return 0;
}

/*
 * m_mode SSCCC M #|SSCCC +|-m
 */
int m_mode(int parc, char **parv)
{
	aNick *nick = GetInfobyPrefix(parv[0]), *nptr = NULL;

	if(parc < 3) return Debug(W_PROTO|W_WARN, "#arg invalide: MODE de %s: #arg=%d", parv[0], parc);
	if(*parv[1] == '#')
	{
		aNChan *chan = GetNChan(parv[1]);
		aChan *ch = NULL;
		char *modes = parv[2], *param = NULL, *key = NULL, *limit = NULL;
		anAccess *a = NULL, *a2 = NULL;
		aJoin *join = NULL;
		aLink *j = NULL;
		int what = 1, i = 2, l = 0, obv = 1;

		if(!chan) return Debug(W_DESYNCH|W_WARN, "M %s de %s: aNChan non trouvé!", parv[1], parv[0]);
		if((ch = chan->regchan) && CSuspend(ch)) ch = NULL; /* don't handle reg stuff if suspended */

		while(*modes)
		{
			switch(*modes)
			{
				case '+':
					what = 1;
					break;
				case '-':
					what = 0;
					break;
				case 'o':
					param = parv[++i];
#ifdef HAVE_OPLEVELS
					if(what)
					{
						char *p = strchr(param, ':');
						if(p) *p = 0;
					}
#endif
					if(!(nptr = num2nickinfo(param)))
					{
						Debug(W_DESYNCH|W_WARN, "m_mode: aNick non trouvé pour '%s %co %s'",
							parv[1], what ? '+' : '-', param);
						break;
					}

					if(ch) /* salon reg */
					{
						if(!what) /* -o */
						{
							if(nptr->user && nick /* la victime a un accès ou est protégée */
							&& nick != nptr && (IsAdmin(nptr->user) || (a = GetAccessIbyUserI(nptr->user, ch))))
							{	/* l'OP n'est pas auth ou lvl inférieur */
								if(!nick->user || (nptr->user->level > nick->user->level ||
								(a && AProtect(a) && !ASuspend(a) && (a2 = GetAccessIbyUserI(nick->user, ch))
								&& !ASuspend(a2) && a2->level <= a->level)))/* ou accés inférieur/suspend */
								{
									csmode(ch, MODE_OBV, "+o $", nptr->numeric);
									break;
								}
							}
							else if(!strcmp(cs.num, param)) /* deop du CServ */
							{ 	/* hack avec la Uline au cas où.. (ircu avec mode non débridé etc.) */
								putserv("%s "TOKEN_MODE" %s +o %s", bot.servnum, ch->chan, cs.num);
								break;
							}
						}
						else /* +o */
						{ /* noops ou strictop */
							if(!IsOperOrService(nptr) && (CNoOps(ch)
							|| (CStrictOp(ch) && (!nptr->user || !(a = GetAccessIbyUserI(nptr->user, ch)) || ASuspend(a)))))
							{
								csmode(ch, MODE_OBV, "-o $", param);
								break;
							}
						}
					} /* mise à jour du status */

					if((join = GetJoinIbyNC(nptr, chan)))
					{
						if(what) DoOp(join);
						else DeOp(join);
					}
					break;

				case 'v':
					param = parv[++i];
					if(ch && what && CNoVoices(ch))
					{
						csmode(ch, MODE_OBV, "-v $", param);
						break;
					} /* mise à jour du status */
					if((nptr = num2nickinfo(param)) && (join = GetJoinIbyNC(nptr, chan)))
					{
						if(what) DoVoice(join);
						else DeVoice(join);
					}
					break;

				case 'b':
					param = parv[++i]; /* grab parameter (mask) */
					/* salon non reg, ou -b, ou service, on ignore, si ban perm, il sera enforcé */
					if(!nick || !what || !ch || nick->flag & N_SERVICE) break;

					if(!CNoBans(ch))
					{
						l = nick->user ? ChanLevelbyUserI(nick->user, ch) : 0;
						if(l >= ch->banlevel)/* peut à priori bannir */
						{
							for(j = chan->members; j; j = j->next)
							{	/* recherche de personnes protégées couvertes */
								nptr = j->value.j->nick;
								if(!nptr->user || match(param, GetNUHbyNick(nptr, 1))) continue;
								if((a = GetAccessIbyUserI(nptr->user, ch)) && AProtect(a) && !ASuspend(a)) break;
								else a = NULL;
							}
						}
					}

					if(CNoBans(ch) || ((ch->banlevel > l || a) && !IsAnAdmin(nick->user)))
						csmode(ch, MODE_OBV, "-b $", param);
					break;

				default: /* other modes than OBV! (Need Parse!) */
					if(*modes == 'l' && what) limit = parv[++i];
					else if(*modes == 'k') key = parv[++i];
#ifdef HAVE_OPLEVELS
					else if(*modes == 'A' || *modes == 'U') ++i; /* ignore A/U passwords */
#endif
					obv = 0;
			} /* switch */
			++modes;
		} /* while */

		/* Only if registered AND some modes other than OBV */
		/* ChModesLevel check (cancel modes change if needed, a bit ugly but work) */
		if(ch && !obv && nick && ch->cml && (!nick->user || (!IsAdmin(nick->user)
			&& (!(a = GetAccessIbyUserI(nick->user, ch)) || ASuspend(a) || a->level < ch->cml))))
			modes_reverse(chan, parv[2], key, limit);

		else if(!obv) string2scmode(&chan->modes, parv[2], key, limit); /* parse */

	} /* if(ch) */
	else if(nick) nick->flag = parse_umode(nick->flag, parv[2]);
	return 0;
}

/*
 * m_part ABAAA L # [:raison]
 */
int m_part(int parc, char **parv)
{
	char *p, *ptr = NULL;
	aNick *nick = NULL;
	aNChan *chan;

	if(parc < 2) return Debug(W_PROTO|W_WARN, "#arg invalide: PART de %s: #arg=%d", parv[0], parc);

	if(!(nick = GetInfobyPrefix(parv[0])))
		return Debug(W_DESYNCH|W_WARN, "PART %s de %s: aNick non trouvé!", parv[1], parv[0]);

	for(p = Strtok(&ptr, parv[1], ','); p; p = Strtok(&ptr, NULL, ','))
	{
		if((chan = GetNChan(p))) del_join(nick, chan);
		else Debug(W_DESYNCH|W_WARN, "PART %s de %s: aNChan non trouvé!", parv[1], parv[0]);
	}
	return 0;
}

/*
 * m_kick SSCCC K # SSCCC :raison
 */
int m_kick(int parc, char **parv)
{
	aNick *k, *v;
	aChan *ch;
	aNChan *chan;
	aJoin *j = NULL;
	anAccess *a1 = NULL, *a2 = NULL;

	if(parc < 3) return Debug(W_PROTO|W_WARN, "#arg invalide: KICK de %s: #arg=%d", parv[0], parc);

	if(!(v = num2nickinfo(parv[2])))
		return Debug(W_DESYNCH|W_WARN, "KICK de %s sur %s par %s: cible introuvable?!", parv[0], parv[2], parv[1]);

	if(!(chan = GetNChan(parv[1])))
		return Debug(W_DESYNCH|W_WARN, "KICK de %s sur %s par %s: aNChan non trouvé!", parv[0], parv[2], parv[1]);

	ch = chan->regchan;
	del_join(v, chan);

	if(!ch || CSuspend(ch)) return 0;

	if(!strcmp(cs.num, parv[2])) /* on ne sait jamais */
	{
		csjoin(ch, JOIN_FORCE);
		return Debug(W_DESYNCH|W_WARN, "Kické de %s par %s!?", ch->chan, parv[0]);
	}

	if(v->user && (k = GetInfobyPrefix(parv[0])) && k != v /* générique */
		&& !(k->flag & (N_SERVICE|N_GOD))
		&& ((IsAdmin(v->user) && (!k->user || k->user->level < v->user->level)) /* v admin > */
			|| ((a1 = GetAccessIbyUserI(v->user, ch)) && AProtect(a1) && !ASuspend(a1)/* OU v a un accès valide*/
				 && (!k->user /* et k non logué */
					 || (!IsAdmin(k->user) /* si auth mais PAS admin */
				 		&& (!(a2 = GetAccessIbyUserI(k->user, ch)) || ASuspend(a2)/* OU n'a pas d'accès valide*/
							|| a2->level <= a1->level)))))) /* OU v protect ET k <= v */
	{
		csmode(ch, MODE_OBV, "-o $", k->numeric);/* pas le droit de kick > déop */
		if((j = GetJoinIbyNC(k, chan))) DeOp(j);
	}
	return 0;
}

/*
 * m_join SSCCC J # TS
 */
int m_join(int parc, char **parv)
{
	aNick *nick = NULL;

	if(parc < 2) return Debug(W_PROTO|W_WARN, "#arg invalide: JOIN de %s: arg=%d", parv[0], parc);

	if(!(nick = GetInfobyPrefix(parv[0])))
		return Debug(W_DESYNCH|W_WARN, "JOIN %s de %s: aNick non trouvé!", parv[1], parv[0]);

	if(*parv[1] == '0') del_alljoin(nick); /* partall */
	else add_join(nick, parv[1], 0,
			parc > 2 ? strtol(parv[2], NULL, 10) : CurrentTS, GetNChan(parv[1]));
	return 0;
}

/*
 * m_create SSCCC C # TS
 */
int m_create(int parc, char **parv)
{
	char *p, *ptr = NULL;
	aNick *nick;

	if(parc < 3) return Debug(W_PROTO|W_WARN, "#arg invalide: CREATE de %s: arg=%d", parv[0], parc);
	if(!(nick = num2nickinfo(parv[0])))
		return Debug(W_DESYNCH|W_WARN, "CREATE %s de %s: aNick non trouvé!", parv[1], parv[0]);

	for(p = Strtok(&ptr, parv[1], ','); p; p = Strtok(&ptr, NULL, ','))
		add_join(nick, p, J_OP|J_CREATE, strtol(parv[2], NULL, 10), NULL);
	return 0;
}

/*
 * m_nick SS N nick (hop) TS ident host.com [+mode [args ...]] IPBASE64 num realname
 */
int m_nick(int parc, char **parv)
{
	char *nick = parv[1], *ac = NULL;
	aNick *who = NULL;
	anUser *user;

	if(parc > 7) /* Nouveau nick */
	{
		aServer *serv = num2servinfo(parv[0]);
						/*  nick, ident	, host	 , base64		,	num	*/
		who = add_nickinfo(nick, parv[4], parv[5], parv[parc-3], parv[parc-2],
						/* aServ*,	real-name ,	timestamp	,	umodes */
							serv, parv[parc-1], atol(parv[3]), *parv[6] == '+' ? parv[6] : NULL);
		if(!who) return 0; /* sauvetage temporaire.. */

		if(*parv[6] == '+')
		{
			int i = 7;
			if(who->flag & N_REG) ac = parv[i++];
			/*if(who->flag & N_SPOOF) spoof = parv[i++];*/
		}

		if(ac)
		{
			char *ac_ts = strchr(ac, ':');

			if(ac_ts) *ac_ts++ = 0; /* ok there is a timestamp */
			if(!(user = getuserinfo(ac)) || user->n)
			{
				cswallops("ACCOUNT %s (%s) pour %s depuis %s", ac,
					user ? "Desynch" : "inconnu", who->nick, serv->serv);
				cs_account(who, NULL);
				return 0;
			}
			who->user = user;
			who->user->n = who;
			who->user->lastseen = CurrentTS;
			if(IsAdmin(user)) adm_active_add(who);
		}

#ifdef USE_WELCOMESERV /* Envoi du welcome si il est actif et non nul & non burst */
		if(GetConf(CF_WELCOME) && !burst && !(serv->flag & ST_BURST)) choose_welcome(who->numeric);
#endif

	}
	else if(parc <= 3) /* Changement de pseudo */
	{
		if(!(who = num2nickinfo(parv[0])))
			return Debug(W_DESYNCH|W_WARN, "NICK %s > %s: aNick non trouvé!", parv[0], nick);

#ifdef USE_NICKSERV
		if(NHasKill(who)) kill_remove(who);
#endif
		switch_nick(who, nick);
	}
	else return Debug(W_PROTO|W_WARN, "#arg invalide: NICK %s %s %s: arg=%d",
					parv[0], parv[1], parv[2], parc);

#ifdef USE_NICKSERV	/* le mec est auth sur ce nick OU nick pris n'est pas reg */
	if((who->user && !strcasecmp(who->user->nick, nick)) || !(user = getuserinfo(nick))) return 0;

	csreply(who, GetReply(who, L_TOOKREGNICK), nick, cs.nick, RealCmd("login"), nick);

	if(IsProtected(user))
	{
		int needkill = (!GetConf(CF_NOKILL) && UPKill(user));

		csreply(who, GetReply(who, needkill ? L_NICKKILLED : L_NICKCHANGED), cf_kill_interval);
		add_killinfo(who, needkill ? TIMER_KREGNICK : TIMER_CHNICK);
	}
#endif
	return 0;
}

/*
 * m_burst SS B # TS [mode key lim] users %ban
 */
int m_burst(int parc, char **parv)
{
	aNick *nick = NULL;
	aChan *c = NULL;
	aNChan *netchan = NULL;
	const char *chan = parv[1];
	char *modes = NULL, *users = NULL, *flags = NULL, *num = NULL, *key = NULL, *limit = NULL;
	int status = 0, i = 4;
	time_t ts = strtol(parv[2], NULL, 10);

	if(parc < 3) return Debug(W_PROTO|W_WARN, "#arg invalide: BURST de %s: arg=%d", parv[0], parc);

	if(parc == 3)
#ifdef HAVE_OPLEVELS
	; /* zannel on .12, handle it to get the right TS */
#else
	return 0;
#endif
	else if(*parv[3] == '+') /* channel has mode(s) */
	{	/* grap modes' argument if needed */
		for(modes = ++parv[3]; *parv[3]; ++parv[3])
			switch(*parv[3])
			{
				case 'l': limit = parv[i++]; break;
				case 'k': key = parv[i++]; break;
#ifdef HAVE_OPLEVELS
				case 'A': case 'U': ++i;
#endif
				default: break;
			}
		users = parv[i];
	}
	else if(*parv[3] != '%') users = parv[3];
	else return 0; /* bans only */

	if(!(netchan = GetNChan(chan))) /* new channel on net */
	{
		netchan = new_chan(chan, ts);
		/* find out if it's registered */
		if((netchan->regchan = getchaninfo(chan))) netchan->regchan->netchan = netchan;
	}
	c = netchan->regchan;

#ifdef HAVE_OPLEVELS
	if(parc == 3) /* Zannel BURST? */
	{
		if(ts < netchan->timestamp && netchan->timestamp <= ts + 4 &&
			(netchan->users || (c && CJoined(c))))
		/* Only do this when WE have users, so that if we do this the BURST that we sent
		   has parc > 3 and the other side will use the test below: */
			ts = netchan->timestamp; /* Do not deop our side. */
	}
	else if(netchan->timestamp < ts && ts <= netchan->timestamp + 4 &&
		!netchan->users && (!c || !CJoined(c)))
	/* If one side of the net.junction does the above
	   timestamp = netchan->timestamp, then the other side must do this: */
		netchan->timestamp = ts;  /* Use the same TS on both sides. */
#endif

	if(!burst)
	{
		if(ts < netchan->timestamp) /* Get an older channel */
		{
			aLink *l = netchan->members;
			netchan->modes.modes = 0U; /* clearmodes */
			netchan->modes.limit = 0U;
			*netchan->modes.key = 0;
			netchan->timestamp = ts;
			/* deopall */
			for(; l; l = l->next) l->value.j->status &= ~(J_OP | J_VOICE);
			/* if registered and on channel, get op back */
			if(c && CJoined(c))
				putserv("%s " TOKEN_MODE " %s +o %s", bot.servnum, netchan->chan, cs.num);
		}
		/* burst d'un salon après EB -> ajout de ses modes (sauf si limite >= à l'actuelle) */
	}
	else /* burst */
	{/* envoi d'un B d'un salon reg par uplink, B-back avec defmodes, ajout des deux séries de modes */
		if(c && !CSuspend(c) && !CJoined(c))
		{
			putserv("%s "TOKEN_BURST" %s %T +%s %s:o", bot.servnum, c->chan, ts,
				GetCModes(c->defmodes), cs.num);
			netchan->modes = c->defmodes; /* c'est ma 1ère connexion, donc aucun mode mémorisé */
			netchan->timestamp = ts;
			do_cs_join(c, netchan, 0);
		}
	}
	/* ajout des modes du salon du réseau, en appliquant les règles en cas de collision */
	if(modes) string2scmode(&netchan->modes, modes, key && HasMode(netchan, C_MKEY)
		&& strcmp(netchan->modes.key, key) <= 0 ? netchan->modes.key : key,
		limit && HasMode(netchan, C_MLIMIT) && strtoul(limit, NULL, 10) >= netchan->modes.limit ? NULL : limit);

	if(!users) return 0; /* zombies' channel... it can exist with extra modes. */
	for(num = Strtok(&key, users, ','); num; num = Strtok(&key, NULL, ','))
	{
		if(*(flags = &num[2*NUMSERV+1]) == ':') /* SSCCC[:[o|v]] recherche directe sans while */
		{
			status = 0;
			*flags = '\0'; /* efface le ':' pour n'avoir que la num */
			if(*++flags) status |= (*flags == 'v') ? J_VOICE : J_OP; /* *flags != 'v' <=> */
			if(*++flags) status |= (*flags == 'v') ? J_VOICE : J_OP; /* *flags == ('o' || :digit:) */
#ifdef HAVE_OPLEVELS
			if(isdigit((unsigned char) *flags)) /* oplevel */
			{
				status |= J_OP; /* *op*levels */
				if(*flags == '0' && !isdigit((unsigned char) flags[1])) status |= J_MANAGER;
				while(isdigit((unsigned char) *++flags)); /* don't use it for now */
			}
#endif
		}
		if((nick = num2nickinfo(num))) add_join(nick, chan, status | J_BURST, ts, netchan);
		else Debug(W_DESYNCH|W_WARN, "BURST %s sur %s de %s: aNick non trouvé!", num, parv[1], parv[0]);
	}

	return 0;
}

/*
 * m_quit SSCCC Q :raison
 */
int m_quit(int parc, char **parv)
{
	del_nickinfo(parv[0], "QUIT");
	return 0;
}

/*
 * m_kill SSCCC D SSCC :path (raison)
 */
int m_kill(int parc, char **parv)
{
	aNick *nick;

	if(parc < 3) return Debug(W_PROTO|W_WARN, "#arg invalide: KILL de %s: arg=%d", parv[0], parc);
	/* Ghost 5 (KILL sent back by victim's *server* <=> prefixlen = 2) */
	if(!(nick = num2nickinfo(parv[1])) && parv[0][2] == 0) return 0;
	if(!nick) return Debug(W_DESYNCH|W_WARN, "KILL de %s sur %s: aNick non trouvé!", parv[0], parv[1]);

	del_nickinfo(parv[1], "KILL");
	return 0;
}

/*
 * m_away SSCCC A [msg]
 */
int m_away(int parc, char **parv)
{
    aNick *nick = GetInfobyPrefix(parv[0]);

	if(!nick) return Debug(W_DESYNCH|W_WARN, "AWAY de %s: aNick non trouvé!", parv[0]);
	if(parc > 1 && *parv[1]) nick->flag |= N_AWAY;
	else
	{
		nick->flag &= ~N_AWAY;
#ifdef USE_MEMOSERV
		if(nick->user) show_notes(nick);
#endif
	}
	return 0;
}

/*
 * m_whois
 */
int m_whois(int parc, char **parv)
{
	aNick *n;
	int remote = 0;

	if(!strcasecmp(parv[2], cs.nick)) n = num2nickinfo(cs.num);
	else n = getnickbynick(parv[2]), remote = 1;

	if(!n) return 0;

	putserv("%s 311 %s %s %s %s * :%s", bot.servnum, parv[0], n->nick, n->ident,
#ifdef HAVE_CRYPTHOST
		remote ? (n->user && IsHidden(n)) ? "hidden" : n->crypt : n->host, n->name);
#else
		(n->user && IsHidden(n)) ? "hidden" : n->host, n->name);
#endif
	putserv("%s 312 %s %s %s :%s", bot.servnum, parv[0], parv[2],
		n->serveur->serv, remote ? n->name : bot.name);
	if(IsOper(n)) putserv("%s 313 %s %s :est un IRC Opérateur %s", bot.servnum, parv[0],
						parv[2], remote ?  "tyrannique et pervers" : "- Service");
	if(!remote) putserv("%s 317 %s %s %T %T :seconds idle, signon time",
					bot.servnum, parv[0], parv[2], CurrentTS - bot.uptime, bot.uptime);
	putserv("%s 318 %s %s :End of /WHOIS list.", bot.servnum, parv[0], parv[2]);
	return 0;
}

int m_topic(int parc, char **parv)
{
	aNChan *chan;

	if(parc < 2) return Debug(W_PROTO|W_WARN, "#arg invalide: TOPIC de %s: arg=%d", parv[0], parc);
	if(!(chan = GetNChan(parv[1])))
		return Debug(W_DESYNCH|W_WARN, "TOPIC de %s sur %s : aNChan non trouvé!", parv[0], parv[1]);

	/* topic was protected, enforce previous topic if any or restore DefTopic*/
	if(chan->regchan && CLockTopic(chan->regchan) && !burst)
		cstopic(chan->regchan, *chan->topic ? chan->topic : chan->regchan->deftopic);
	else Strncpy(chan->topic, parv[parc-1], TOPICLEN); /* update DB */
	return 0;
}

/*
 * m_eob SS EB SS
 */
int m_eob(int parc, char **parv)
{
	aServer *serv = num2servinfo(parv[0]);

	if (!serv || !(serv->flag & ST_BURST))
		return Debug(W_DESYNCH|W_WARN, "EOB de %s: serveur inconnu/déjà synchro?", parv[0]);

	serv->flag &= ~ST_BURST;
	serv->flag |= ST_ONLINE;
	if(serv == mainhub) /* mon uplink a fini son burst, j'envoie la fin du mien! */
	{
		aChan *chan;
		aNChan *netchan = GetNChan(bot.pchan);
		int i = 0;

		/* join log channel (with J_BURST flag, new_chan() will be called silently) */
		add_join(num2nickinfo(cs.num), bot.pchan, J_OP|J_BURST, CurrentTS, netchan);
		/* if channel was empty, first called failed */
		if(!netchan) netchan = GetNChan(bot.pchan);

		string2scmode(&netchan->modes, "mintrs", NULL, NULL);
		putserv("%s "TOKEN_BURST" %s %T +mintrs %s:o", bot.servnum, bot.pchan,
			netchan->timestamp, cs.num);

		/* Now burst registered channels which does not exist on the net (+si ones) */
		for(; i < CHANHASHSIZE; ++i) for(chan = chan_tab[i]; chan; chan = chan->next)
		{
			if(!CJoined(chan) && HasDMode(chan, C_MINV|C_MKEY) && !CSuspend(chan))
			{
				putserv("%s " TOKEN_BURST " %s %T +%s %s:o", bot.servnum,
					chan->chan, CurrentTS, GetCModes(chan->defmodes), cs.num);

				do_cs_join(chan, (netchan = new_chan(chan->chan, CurrentTS)), JOIN_TOPIC);
				chan->netchan = netchan, netchan->regchan = chan;
				netchan->modes = chan->defmodes; /* channel was empty, modes are now defmodes */
			} /* else, try to enforce deftopic on topic-less channels */
			else if(CJoined(chan) && !*chan->netchan->topic && *chan->deftopic)
				cstopic(chan, chan->deftopic);
		}/* end foreach chan */

		putserv("%s " TOKEN_EOB, bot.servnum);
		putserv("%s " TOKEN_EOBACK, bot.servnum);

		if(bot.dataQ)
		{
			struct timeval tv;
			double tt;
			gettimeofday(&tv, NULL);/* un peu de stats sur le burst pour *fun* */
			tt = (tv.tv_usec - burst) / 1000000.0;
			cswallops("Synchronisation de %lu bytes en %.6f s (Taux %.6f Ko/s)",
						bot.dataQ, tt, (bot.dataQ / 1024.0) / tt);
		}
		check_accounts();
		burst = 0;
	}
	return 0;
}

int m_server(int parc, char **parv)
{
	if(parc < 7) return Debug(W_PROTO|W_WARN, "#arg invalide: SERVER de %s: arg=%d", parv[0], parc);

	return add_server(parv[1], parv[6], parv[2], parv[5], parv[0]);
}

static int do_squit(int servnum)
{
	unsigned int i = 0U;

	for(; i < ASIZE(serv_tab); ++i)
		if(serv_tab[i] && serv_tab[i]->hub == serv_tab[servnum]) /* i est un leaf de servnum */
			do_squit(i); /* squit récursif de ses leaf */

	for(i = 0U; i < serv_tab[servnum]->maxusers; ++i)
	{
		if(num_tab[servnum][i]) /* purge des clients de 'i' */
		{
			del_nickinfo(num_tab[servnum][i]->numeric, "SQUIT");
			num_tab[servnum][i] = NULL;
		}
	}

	free(num_tab[servnum]);
	num_tab[servnum] = NULL;
	free(serv_tab[servnum]);
	serv_tab[servnum] = NULL;
	return 1;
}

int m_squit(int parc, char **parv)
{
	int i = 0;

	if(parc < 2) return Debug(W_PROTO|W_WARN, "#arg invalide: SQUIT de %s: arg=%d", parv[0], parc);

	for(; i < MAXNUM; ++i)
		if(serv_tab[i] && !strcasecmp(serv_tab[i]->serv, parv[1]))
			return do_squit(i);

	return Debug(W_DESYNCH|W_WARN, "Squit d'un serveur inconnu %s!", parv[1]);
}

int m_pass(int parc, char **parv)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	burst = tv.tv_usec; /* TS en µs pour les stats */

	log_write(LOG_MAIN, 0, "Connecté au serveur %s", bot.ip);
	bot.lasttime = tv.tv_sec;
	bot.lastbytes = bot.dataQ;
	return 0;
}

int m_ping(int parc, char **parv)
{
	struct timeval tv;
	char *theirttm;

	if(parc < 4)
	{
		putserv("%s " TOKEN_PONG " %s", bot.servnum, parv[1]);
		return 0;
	}
	if((theirttm = strchr(parv[3], '.'))) ++theirttm;
	gettimeofday(&tv, NULL);
/* AsLL */
	putserv("%s " TOKEN_PONG " %s %s %s %d %T.%T", bot.servnum, bot.servnum, parv[0],
		parv[3], ((tv.tv_sec - atoi(parv[3])) * 1000 + (tv.tv_usec - atoi(theirttm)) / 1000),
		tv.tv_sec, tv.tv_usec);/* mon TS seconde.micro */
    return 0;
}

int m_error(int parc, char **parv)
{
	return Debug(W_MAX|W_TTY, "Erreur reçue du Hub (%s)", parv[1]);
}

static int exec_cmd(aHashCmd *cmd, aNick *nick, int parc, char **parv)
{
	char *tmp = NULL, buff[512];
	aChan *chan = NULL;
	anAccess *acces = NULL;

	if(isignore(nick) || checkflood(nick)) return 0;

	++cmd->used;
	if(DisableCmd(cmd)) return csreply(nick, GetReply(nick, L_CMDDISABLE));

	if(parc < cmd->args || (!parc && ChanCmd(cmd)))	return syntax_cmd(nick, cmd);

	if(ChanCmd(cmd))
	{
		if(*parv[1] != '#' || !(chan = getchaninfo(parv[1])))
			return csreply(nick, GetReply(nick, L_NOSUCHCHAN), parv[1]);

		if(CSuspend(chan) && cmd->level)
			return csreply(nick, "%s est suspendu : %s - %s", parv[1],
					chan->suspend->raison, chan->suspend->from);

		if(!chan->netchan && NeedMemberShipCmd(cmd))
			return csreply(nick, GetReply(nick, L_NEEDMEMBERSHIP), parv[1]);
	}
	if(cmd->level)
	{
		if(!nick->user) return csreply(nick, "%s", cf_pasdeperm);

		if(AdmCmd(cmd) && nick->user->level < cmd->level)
			return csreply(nick, GetReply(nick, L_NEEDTOBEADMIN));

		if(ChanCmd(cmd) && !IsAdmin(nick->user))
		{
			if(!(acces = GetAccessIbyUserI(nick->user, chan)))
				return csreply(nick, GetReply(nick, L_YOUNOACCESSON), parv[1]);

			if(cmd->level > acces->level)
				return csreply(nick, GetReply(nick, L_ACCESSTOOLOW), parv[1], acces->level, cmd->level);

			if(ASuspend(acces))
				return csreply(nick, GetReply(nick, L_ACCESSSUSPENDED), parv[1]);
		}
	}

	if(!ChanCmd(cmd) && parc && (SecureCmd(cmd) || (parc >= 2 && Secure3Cmd(cmd)
		&& (!strcasecmp(parv[1], "send") || !strcasecmp(parv[1], "pass")
			|| !strcasecmp(parv[1], "newpass")))))
		tmp = parv[1], tmp[400] = 0; /* truncate the buffer otherwise it could overflow 'buff' */
	else if((ChanCmd(cmd) || !Secure2Cmd(cmd)) && parc)
		tmp = parv2msg(parc, parv, 1, 400);

	if(nick->user) fastfmt(buff, "\2$\2 $ par $@$", cmd->name, tmp, nick->nick, nick->user->nick);
	else fastfmt(buff, "\2$\2 $ par $", cmd->name, tmp, nick->nick);

#ifdef HAVE_TRACK
	if(nick->user && UTracked(nick->user)) track_notify(nick->user, buff);
#endif

	putchan(buff);
	log_write(ChanCmd(cmd) ? LOG_CCMD : LOG_UCMD, 0, "%s", buff);
	cmd->func(nick, chan, parc, parv); /* passage du aChan * en plus, pour économie */
	return 1;
}

/*
 * m_privmsg SSCCC P SSCCC|# :msg
 * pparv[0] = nom de la commande
 * pparv[pparc] = arguments de la commande
 * ATTENTION: pparc est le nb d'arg à partir de *parv[1]*
 */
int m_privmsg(int parc, char **parv)
{
	char *tmp, *pparv[250], *ptr = NULL;
	unsigned int pparc = 0;
	aHashCmd *cmd;
	aNick *nptr = GetInfobyPrefix(parv[0]);

	if(!nptr || !*parv[2]) return 0;/* unknown source or no param */

	if(*parv[1] != '#') /* Message privé */
	{
		int secure = 0;
		pparv[1] = NULL;

		if((tmp = strchr(parv[1], '@'))) /* P nick@server */
		{
			*tmp = 0;
			secure = 1;
			if(strcasecmp(parv[1], cs.nick)) return 0;
		}
		else if(strcmp(parv[1], cs.num)) return 0; /* not directed to me */

		if(!(tmp = Strtok(&ptr, parv[2], ' '))) return 0;

		if(!(cmd = FindCommand(tmp)))
		{
			if(*tmp != '\1') csreply(nptr, GetReply(nptr, L_NOSUCHCMD), tmp);
			return 0;
		}

#ifdef HAVE_SECURE
		if(SecureCmd(cmd) && !secure) return csreply(nptr, GetReply(nptr, L_SECURECMD),
			cmd->name, cs.nick,	cmd->name, ptr ? ptr : "");
#endif
		pparv[0] = tmp;
		if(*tmp != '\1') for(; (tmp = Strtok(&ptr, NULL, ' ')); ) pparv[++pparc] = tmp;
		else if(ptr) pparv[++pparc] = ptr; /* CTCP */

		exec_cmd(cmd, nptr, pparc, pparv);
	}
	else if(*parv[2] == bot.cara && *++parv[2] && *parv[2] != ' ')
	{	/* Message envoyé sur un salon, recherche d'une commande (quel flood..) */
		pparv[0] = (tmp = Strtok(&ptr, parv[2], ' '));

		if(!tmp || !(cmd = FindCommand(tmp))) return 0; /* pas d'arg ou commande inconnue */

		tmp = Strtok(&ptr, NULL, ' ');
		if(ChanCmd(cmd) && (!tmp || *tmp != '#')) pparv[++pparc] = parv[1]; /* next arg is a channel */
		if(tmp) pparv[++pparc] = tmp; /* otherwise use the one command is typed in */

		while((tmp = Strtok(&ptr, NULL, ' '))) pparv[++pparc] = tmp;

		exec_cmd(cmd, nptr, pparc, pparv);
	}
	return 0;
}
