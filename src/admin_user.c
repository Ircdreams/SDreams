/* src/admin_user.c - commandes admins pour gerer les users
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
 * $Id: admin_user.c,v 1.153 2007/12/16 20:48:15 romexzf Exp $
 */

#include <sys/time.h>
#include "main.h"
#include "outils.h"
#include "hash.h"
#include "cs_cmds.h"
#include "fichiers.h"
#include "divers.h"
#include "crypt.h"
#include "userinfo.h"
#include "admin_user.h"
#include "data.h"

int cs_whois(aNick *nick, aChan *chan, int parc, char **parv)
{
	char chans[400] = {0};
	size_t size = 0, first_row = 1;
	anUser *uptr;
	aJoin *join;
	aNick *nptr = getnickbynick(parv[1]);

	if(!nptr) return csreply(nick, GetReply(nick, L_NOSUCHNICK), parv[1]);

	csreply(nick, "Infos sur \2%s", nptr->nick);
	csreply(nick, " Nom: %s", nptr->name);
#ifdef HAVE_CRYPTHOST
	csreply(nick, " Mask: %s\2%s\2!\002%s\2@\002%s", GetPrefix(nptr), nptr->nick,
		nptr->ident, nptr->crypt);
	csreply(nick, " Realhost: \002%s\2", nptr->host);
#else
	csreply(nick, " Mask: %s\2%s\2!\002%s\2@\002%s", GetPrefix(nptr), nptr->nick,
		nptr->ident, nptr->host);
#endif
	csreply(nick, " IP:\2 %s", GetUserIP(nptr, NULL));
	if(IsOper(nptr)) csreply(nick, " %s est \2IRCop.", nptr->nick);
	if(nptr->flag & N_UMODES) csreply(nick, " UserModes: \2%s\2", GetModes(nptr->flag));

	for(join = nptr->joinhead; join; join = join->next)
	{
		size_t chan_len = strlen(join->chan->chan);

		if(size + chan_len + 4 >= sizeof chans - 1) /* next one would overflow the buffer */
		{
			csreply(nick, first_row ? " Présent sur %s..." : "%s..." , chans);
			first_row = size = 0; /* buf has been flushed, go back to the begninning */
		}
		/* prefix */
		if(IsVoice(join)) chans[size++] = '+';
		if(IsOp(join)) chans[size++] = '@';
		/* channel and suffix */
		strcpy(chans + size, join->chan->chan);
		size += chan_len;
		chans[size++] = ' ';
		chans[size] = 0;
	}

	csreply(nick, first_row ? " Présent sur %s" : "%s", *chans ? chans : "aucun salon.");

	if(nptr->user) csreply(nick, " %s est logué sous l'username \2%s\2%s", nptr->nick,
			nptr->user->nick, IsAdmin(nptr->user) ? " (\2Admin\2)" : "");

	else if((uptr = getuserinfo(parv[1]))) csreply(nick, " %s est un username enregistré. %s",
				uptr->nick, IsAdmin(uptr) ? " (\2Admin\2)" : "");

	csreply(nick, " Connecté depuis %s sur \2%s", duration(CurrentTS - nptr->ttmco),
		nptr->serveur->serv);
	return 1;
}

int admin_user(aNick *nick, aChan *chan, int parc, char **parv)
{
	char *user = parv[2];
	const char *arg = parv[1];
	anUser *u = NULL;
	aNick *n;

	if(!strcasecmp(arg, "info"))
	{ 	/* critères de recherche */
		int mail = getoption("-mail", parv, parc, 2, GOPT_STR);
		int seen = getoption("-seen", parv, parc, 2, GOPT_STR);
		int date = getoption("-bseen", parv, parc, 2, GOPT_STR);
		int max = getoption("-count", parv, parc, 2, GOPT_INT);
		int auth = getoption("-auth", parv, parc, 2, GOPT_FLAG);
		int suspend = getoption("-suspend", parv, parc, 2, GOPT_FLAG);
		int last = getoption("-last", parv, parc, 2, GOPT_STR);
		int level = getoption("-level", parv, parc, 2, GOPT_INT), flags = 0, found = 0, i = 0;
		time_t seent = 0, datet = 0;
		/* convert 'seen' duration to the lower bound TS */
		if(seen > 0 && (seent = convert_duration(parv[seen]))) seent = CurrentTS - seent;
		/* then convert 'bseen' duration to the higher bound TS */
		if(date > 0 && (datet = convert_duration(parv[date]))) datet = CurrentTS - datet;

		if(getoption("-first", parv, parc, 2, GOPT_FLAG)) flags |= U_FIRST;
		if(getoption("-oubli", parv, parc, 2, GOPT_FLAG)) flags |= U_OUBLI;
		if(getoption("-nopurge", parv, parc, 2, GOPT_FLAG)) flags |= U_NOPURGE;

		if(*parv[2] == '-') user = NULL; /* look for criteria(s) in all users */
		else if(!HasWildCard(user))
			return (u = ParseNickOrUser(nick, user)) ? show_userinfo(nick, u, 1) : 0;

		if(!max) max = strcmp("-all", parv[parc]) ? MAXMATCHES : -1;

		for(; i < USERHASHSIZE; ++i) for(u = user_tab[i]; u; u = u->next)
		{
			if((!mail || !match(parv[mail], u->mail))
			&& (!seent || u->lastseen >= seent)
			&& (!datet || u->lastseen < datet)
			&& (!auth || u->n)
			&& (!suspend ||USuspend(u))
			&& (!flags || (u->flag & flags) == flags)
			&& (!user || !match(user, u->nick))
			&& (!last || (u->lastlogin && !match(parv[last], u->lastlogin)))
			&& (!level || u->level >= level)
			&& (++found <= max || max < 0))
			{
				show_userinfo(nick, u, 1);
				csreply(nick, "-");
			}
		}

		if(found > MAXMATCHES && max == MAXMATCHES)
			csreply(nick, GetReply(nick, L_EXCESSMATCHES), found, MAXMATCHES);
		if(max > 0 && max < found)
			csreply(nick, "Un Total de %d entrées trouvées (%d listées)", found, max);
		else if(found) csreply(nick, "Un Total de %d entrées trouvés",	found);
		else csreply(nick, "Aucun Username correspondant à\2 %s\2 trouvé.", user ? user : "*");
	}

	else if(!strcasecmp(arg, "match"))
	{
		const char *p = parv[2];
		int w = 1, host = 0, nicki = 0, ident = 0, real = 0, join = 0, par = 2;
		int mode = 0, ac = 0, count = 0, crypt = 0, serv = 0, nolimit = 0, i = 0;
		aServer *link = NULL;
		aNChan *nc = NULL;

		if(*p != '+' && *p != '-')
			return csreply(nick, "Syntaxe %s MATCH [+|-]nughCcsam [args..]", parv[0]);

#define ADMUSR_PARSE_MATCH(x) 	if(((x) = ++par) <= parc) { if(!w) (x) = -(x); } \
								else err = 1; \
								break; \

		for(; *p; ++p)
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
#ifdef HAVE_CRYPTHOST
				case 'C': ADMUSR_PARSE_MATCH(crypt);
#endif
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
#ifdef HAVE_CRYPTHOST
				&& (!crypt || (crypt > 0 && !match(parv[crypt], n->crypt)) || (crypt < 0 && match(parv[-crypt], n->crypt)))
#endif
				&& (!join || (join > 0 && GetJoinIbyNC(n, nc)) || (join < 0 && !GetJoinIbyNC(n, nc)))
				&& (++count <= MAXMATCHES || nolimit))
					csreply(nick, "Matching Nick: [%d] \2%s%s\2 -> %s@%s", count, GetPrefix(n), n->nick, n->ident, n->host);

		if(count > MAXMATCHES && !nolimit)
			csreply(nick, GetReply(nick, L_EXCESSMATCHES), count, MAXMATCHES);

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
		/* if he isn't suspend, but used to be, we need to free it to recreate it */
		if(u->suspend && !USuspend(u)) data_free(u->suspend), u->suspend = NULL;

		switch(data_handle(u->suspend, nick->user->nick, ptr,
							timeout, DATA_T_SUSPEND_USER, u))
		{
			case -1: /* error */
				return csreply(nick, "Veuillez préciser une raison pour suspendre cet UserName.");
			case 0: /* deleted */
				return csreply(nick, "L'Username \2%s\2 n'est plus suspendu.", u->nick);
			case 1: /* created */
				if((n = u->n))
				{
					csreply(n, "Votre Username vient d'être suspendu par l'Admin %s.", nick->nick);
					cs_account(n, NULL);
				}
			case 2: /* updated */
				show_ususpend(nick, u); /* report.. */
		}
	}

	else if(!strcasecmp(arg, "cantregchan"))
	{
		if(!strcasecmp(nick->user->nick, user))
			return csreply(nick, "Vous ne voulez plus pouvoir enregistrer de chan ?");

		if(!(u = ParseNickOrUser(nick, user))) return 0;

		if(IsAdmin(u))
			return csreply(nick, "Vous ne pouvez pas empecher un Admin d'enregistrer des salons.");

		if(parc >= 3 || u->cantregchan) /* do more parsing */
		{
			time_t timeout = 0;
			char *ptr = NULL;

			if(*parv[3] == '%' && (timeout = convert_duration(++parv[3])) <= 0)
				return csreply(nick, GetReply(nick, L_INCORRECTDURATION));

			ptr = parv2msg(parc, parv, timeout ? 4 : 3, 250);

			switch(data_handle(u->cantregchan, nick->user->nick, ptr,
								timeout, DATA_T_CANTREGCHAN, u))
			{
				case 0: /* deleted */
					return csreply(nick, "L'Username \2%s\2 peut maintenant enregistrer des salons.",
								u->nick);
				case 1: /* created */
				case 2: /* updated */
					show_cantregchan(nick, u); /* report.. */
			}
		}
		else switch_option(nick, NULL, arg, u->nick, &u->flag, U_CANTREGCHAN);
	}

	else if(!strcasecmp(arg, "nopurge"))
	{

		if(!(u = ParseNickOrUser(nick, user))) return 0;

		if(parc >= 3 || u->nopurge) /* do more parsing */
		{
			time_t timeout = 0;
			char *ptr = NULL;

			if(*parv[3] == '%' && (timeout = convert_duration(++parv[3])) <= 0)
				return csreply(nick, GetReply(nick, L_INCORRECTDURATION));

			ptr = parv2msg(parc, parv, timeout ? 4 : 3, 250);

			switch(data_handle(u->nopurge, nick->user->nick, ptr,
						timeout, DATA_T_NOPURGE, u))
			{
				case 0: /* deleted */
					return csreply(nick, "L'option NoPurge est désactivée pour %s",	u->nick);
				case 1: /* created */
				case 2: /* updated */
					show_nopurge(nick, u); /* report.. */
			}
		}
		else switch_option(nick, NULL, arg, u->nick, &u->flag, U_NOPURGE);
	}

	else if(!strcasecmp(arg, "mail"))
	{
		if(parc < 3 || !IsValidMail(parv[3]))
			return csreply(nick, GetReply(nick, L_MAIL_INVALID));

		if(!(u = ParseNickOrUser(nick, user))) return 0;

		switch_mail(u, parv[3]);
		csreply(nick, GetReply(nick, L_OKCHANGED));
	}

	else if(!strcasecmp(arg, "newpass"))
	{
		if(parc < 3 || strlen(parv[3]) < 6) return csreply(nick, GetReply(nick, L_PASS_LIMIT));

		if(!(u = ParseNickOrUser(nick, user))) return 0;

		if(IsAdmin(u) && u->level >= nick->user->level && u != nick->user)
			return csreply(nick, GetReply(nick, L_GREATERLEVEL), u->nick);

		password_update(u, parv[3], 0);
		csreply(nick, GetReply(nick, L_OKCHANGED));
	}

	else if(!strcasecmp(arg, "newnick"))
	{
		if(parc < 3 ) return csreply(nick, "Syntaxe: %s NEWNICK <username> <new>", parv[0]);

		if(strlen(parv[3]) > NICKLEN || !IsValidNick(parv[3]))
			return csreply(nick, GetReply(nick, L_USER_INVALID), parv[3]);

		if(getuserinfo(parv[3])) return csreply(nick, GetReply(nick, L_ALREADYREG), parv[3]);
		if(!(u = getuserinfo(user))) return csreply(nick, GetReply(nick, L_NOSUCHUSER), user);

		if(IsAdmin(u) && u->level >= nick->user->level && u != nick->user)
			return csreply(nick, "Vous ne pouvez pas changer l'username d'un Admin de niveau supérieur ou égal.");

		switch_user(u, parv[3]);
		csreply(nick, GetReply(nick, L_OKCHANGED));
	}

	else if(!strcasecmp(arg, "del"))
	{
		if(!strcasecmp(nick->user->nick, user))
			return csreply(nick, "Utilisez la commande \2%s\2.", RealCmd("drop"));

		if(!(u = ParseNickOrUser(nick, user))) return 0;

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
			mysnprintf(tmp, sizeof tmp, "Suppression de l'username de l'owner (%s) par %s",
				u->nick, nick->user->nick);
			del_regnick(u, HF_LOG, tmp);
		}
		else del_regnick(u, HF_LOG, NULL);
	}
	else if(!strcasecmp(arg, "stats"))
	{
		int memn = getoption("-mem", parv, parc, 2, GOPT_FLAG);
		int cmds = getoption("-cmds", parv, parc, 2, GOPT_STR);
		int traffic = getoption("-traffic", parv, parc, 2, GOPT_FLAG);
		int all = getoption("-all", parv, parc, 2, GOPT_FLAG);

		if(memn || all)
		{
			aBan *ban;
			aNChan *nchan;
			aLink *lp;
			aMemo *m;
			int nick_count = 0, user_count = 0, data_count = 0, chan_count = 0,
				nchan_count = 0, member_count = 0, auth_count = 0, memo_count = 0,
				longest, i, used, list_size, ma, mb, maxbans, maxaccess, mem = 0;

			if(!(maxbans = getoption("-maxbans", parv, parc, 2, GOPT_INT))) maxbans = WARNACCESS;
			if(!(maxaccess = getoption("-maxaccess", parv, parc, 2, GOPT_INT))) maxaccess = WARNACCESS;

			uptime(nick, chan, parc, parv);

#define show_hash_stat(type, size, count) 	csreply(nick, "Hash "type": Offsets (Utilisés " \
	"/ Dispos) %d / %d (Max. %d Av. %.3f)", used, size, longest, (double) count / used)

			for(i = 0, longest = 0, used = 0; i < NICKHASHSIZE; ++i)
			{
				for(list_size = 0, n = nick_tab[i]; n; n = n->next, ++list_size);
				if(list_size > longest) longest = list_size;
				if(list_size) ++used;
				nick_count += list_size;
			}
			show_hash_stat("Nick", NICKHASHSIZE, nick_count);

			for(i = 0, longest = 0, used = 0; i < NCHANHASHSIZE; ++i)
			{
				for(list_size = 0, nchan = nchan_tab[i]; nchan; nchan = nchan->next, ++list_size)
					member_count += nchan->users;
				nchan_count += list_size;
				if(list_size > longest) longest = list_size;
				if(list_size) ++used;
			}
			show_hash_stat("NetChan", NCHANHASHSIZE, nchan_count);

			for(i = 0, longest = 0, used = 0; i < USERHASHSIZE; ++i)
			{
				for(list_size = 0, u = user_tab[i]; u; u = u->next, ++list_size)
				{
					for(m = u->memohead; m; m = m->next) ++memo_count;
					if(u->suspend) ++data_count;
					if(u->nopurge) ++data_count;
					if(u->cantregchan) ++data_count;
					if(u->lastlogin) mem += strlen(u->lastlogin);
					if(u->n) ++auth_count;
				}
				if(list_size > longest) longest = list_size;
				if(list_size) ++used;
				user_count += list_size;
			}
			show_hash_stat("User", USERHASHSIZE, user_count);

			for(i = 0, longest = 0, used = 0; i < CHANHASHSIZE; ++i)
			{
				for(list_size = 0, chan = chan_tab[i]; chan; chan = chan->next, ++list_size)
				{
					if(chan->suspend) ++data_count;
					if(chan->motd) mem += strlen(chan->motd);

					for(mb = 0, ban = chan->banhead; ban; ban = ban->next, ++mb)
						if(ban->raison) mem += strlen(ban->raison);
					if(mb >= maxbans)
						csreply(nick, "Warning Bans Limite: \2\00304%d\2\3 Bans sur %s", mb, chan->chan);

					for(ma = 0, lp = chan->access; lp; lp = lp->next, ++ma)
						if(lp->value.a->info) mem += strlen(lp->value.a->info);
					if(ma >= maxaccess)
						csreply(nick, "Warning Acces Limite: \2\00304%d\2\3 Accès sur %s", ma, chan->chan);
					mem += ma * (sizeof(anAccess) + sizeof(aLink)) + mb * sizeof(aBan);
				}
				if(list_size > longest) longest = list_size;
				if(list_size) ++used;
				chan_count += list_size;
			}
			show_hash_stat("Chan", CHANHASHSIZE, chan_count);

			mem += nick_count * sizeof(aNick) + user_count * sizeof(anUser) + chan_count
				* sizeof(aChan) + nchan_count * sizeof(aNChan) + member_count *
				(sizeof(aLink) + sizeof(aJoin))	+ memo_count * sizeof(aMemo)
				+ data_count * sizeof(aData) + (NCHANHASHSIZE + MAXNUM
					+ USERHASHSIZE + CHANHASHSIZE + NICKHASHSIZE + CMDHASHSIZE) * sizeof(void *);

			csreply(nick, "Memoire Utilisée pour les Users/Chans: %.3f KB", mem / 1024.0);
			csreply(nick, "Il y a %d Usernames et %d Chans enregistrés.", user_count, chan_count);
			csreply(nick, "%d salons formés, %d Chatteurs connectés, dont %.1f %% logués.",
				nchan_count, nick_count, (double) auth_count / nick_count * 100);
		}
		if(cmds || all)
		{
			int x = 0, adm = 0, usr = 0;
			aHashCmd *cmd;

			for(; x < CMDHASHSIZE; ++x) for(cmd = cmd_hash[x]; cmd; cmd = cmd->next)
			{
				if(AdmCmd(cmd)) adm += cmd->used;
				else usr += cmd->used;

				if(cmds && !IsCTCP(cmd) && !match(parv[cmds], cmd->name))
					csreply(nick, "- %s (\002%d\2)", cmd->name, cmd->used);
			}
			csreply(nick, "Un total de %d commandes utilisées, dont %d Admins (Ratio: %.2f%)",
				(adm + usr), adm, (adm / (double) (usr + adm)) * 100);
		}
		if(traffic || all)
		{
			csreply(nick, "Traffic: Up:\2 %.3f MB \2 Down:\2 %.3f MB\2",
				bot.dataS/1048576.0, bot.dataQ/1048576.0);
			csreply(nick, "Traffic Actuel: %.3f Ko/s",
				(bot.dataQ - bot.lastbytes) / ((CurrentTS - bot.lasttime) * 1024.0));
#	ifdef WEB2CS
			csreply(nick, "Web2CS: Traffic Up:\2 %.3f Ko\2 Down:\2 %.3f Ko\2 in %d connections",
				bot.WEBtrafficUP/1024.0, bot.WEBtrafficDL/1024.0, bot.CONtotal);
#	endif
		}
		if(!all && !memn && !traffic && !cmds)
			csreply(nick, GetReply(nick, L_UNKNOWNOPTION), parc < 2 ? "<NULL>" : parv[2]);
	}
	else if(!strcasecmp(arg, "userset"))
	{
		time_t reg;
		char *p = NULL;

		if(!(u = ParseNickOrUser(nick, user))) return 0;
		if(parc < 3 || (reg = strtol(parv[3], &p, 10)) <= 0 || (p && *p) || reg > CurrentTS)
			return csreply(nick, "TS non valide");

		u->reg_time = reg;

		csreply(nick, "Date d'enregistrement de %s mise à %s",
			user, get_time(nick, u->reg_time));
	}
	else return csreply(nick, GetReply(nick, L_UNKNOWNOPTION), arg);
	return 1;
}
